#define DLL_EXPORT
#include "soundsystem.h"
#include "engine.h"

#define RG_SOURCEPOOL_SIZE 128

namespace Engine {

	enum SourceState {
		SourceState_INITIAL = 0,
		SourceState_PLAYING
	};

	struct Source {
		ALuint       source;
		SourceState  srcstate;
		SoundBuffer* buffer;
	};


	/////////////////////////////////////////////////////
	// Sound system
	/////////////////////////////////////////////////////
	SoundSystem::SoundSystem() {
		rgLogInfo(RG_LOG_SYSTEM, "Starting up sound system...");
		m_sourcepool = (Source*)GetDefaultAllocator()->Allocate(sizeof(Source) * RG_SOURCEPOOL_SIZE);
		m_alloc      = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Sound source pool", RG_SOURCEPOOL_SIZE, sizeof(SoundSource));

		m_device = alcOpenDevice(NULL);
		m_ctx    = alcCreateContext(m_device, NULL);
		if (!alcMakeContextCurrent(m_ctx)) {
			RG_ERROR_MSG("Failed to create ALC context!");
		}

		// Setup OpenAL
		alListenerf(AL_GAIN, m_volume);

		// Make sources
		for (Uint32 i = 0; i < RG_SOURCEPOOL_SIZE; i++) {
			alGenSources(1, &m_sourcepool[i].source);
			m_sourcepool[i].buffer = NULL;
		}


	}

	SoundSystem::~SoundSystem() {
		for (Uint32 i = 0; i < RG_SOURCEPOOL_SIZE; i++) {
			alDeleteSources(1, &m_sourcepool[i].source);
		}

		GetDefaultAllocator()->Deallocate(m_sourcepool);
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, m_alloc);

		alcDestroyContext(m_ctx);
		alcCloseDevice(m_device);
	}

	SoundBuffer* SoundSystem::CreateBuffer(SoundBufferCreateInfo* info) {
		return new SoundBuffer(info); // TMP
	}

	void SoundSystem::DestroyBuffer(SoundBuffer* ptr) {
		delete ptr; // TMP
	}

	SoundSource* SoundSystem::NewSoundSource() {
		SoundSource* comp = RG_NEW_CLASS(m_alloc, SoundSource)();
		m_sourcecomponents.push_back(comp);
		return comp;
	}

	void SoundSystem::DeleteSoundSource(SoundSource* comp) {
		std::vector<SoundSource*>::iterator it = m_sourcecomponents.begin();
		for (; it != m_sourcecomponents.end(); it++) {
			if (*it == comp) {
				*it = std::move(m_sourcecomponents.back());
				m_sourcecomponents.pop_back();
				//m_sourcecomponents.erase(it);
				RG_DELETE_CLASS(m_alloc, SoundSource, comp);
				break;
			}
		}
	}

	void SoundSystem::PlaySound(PlaySoundInfo* info) {

		// Request sound source
		Source* src = RequestSource();
		if (!src) {
			rgLogError(RG_LOG_SYSTEM, "No sound source available.");
		}

		src->srcstate = SourceState_PLAYING;
		src->buffer   = info->buffer;

		alSourcei(src->source,  AL_BUFFER,   src->buffer->GetBuffer());

		alSourcef(src->source,  AL_PITCH,    info->speed);
		alSourcef(src->source,  AL_GAIN,     info->volume);
		alSourcei(src->source,  AL_LOOPING,  AL_FALSE);
		alSourcefv(src->source, AL_POSITION, info->position.array);
		alSource3f(src->source, AL_VELOCITY, 0, 0, 0);

		alSourcePlay(src->source);
		
	}

	void SoundSystem::SetListenerPosition(const vec3& pos) {
		alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
	}

	void SoundSystem::SetListenerVelocity(const vec3& vel) {
		alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
	}

	void SoundSystem::SetListenerOrientation(const vec3& fwd, const vec3& up) {
		ALfloat orient[] = {
			fwd.x, fwd.y, fwd.z,
			up.x, up.y, up.z
		};
		alListenerfv(AL_ORIENTATION, orient);
	}

	void SoundSystem::Update(Float64 dt) {

		// Update
		if (m_camera != NULL) {
			Transform* t_cam = m_camera->GetTransform();

			vec3 vel = { 0, 0, 0 };
			vec3 pos = t_cam->GetPosition();
			vec3 rot = t_cam->GetRotation();
			vec3 up  = { 0, 1, 0 };

			// TODO: rewrite this
			vec3 fwd;
			fwd.x = SDL_sinf(rot.y);
			fwd.y = -SDL_tanf(rot.x);
			fwd.z = -SDL_cosf(rot.y);

			this->SetListenerPosition(pos);
			this->SetListenerVelocity(vel);
			this->SetListenerOrientation(fwd, up);
		}

		ALint ival = 0;
		for (Uint32 i = 0; i < RG_SOURCEPOOL_SIZE; i++) {
			Source* src = &m_sourcepool[i];
			// Get state
			alGetSourcei(src->source, AL_SOURCE_STATE, &ival);
			
			if (ival == AL_STOPPED && src->srcstate == SourceState_PLAYING) {
				// Free source if play stopped
				src->srcstate = SourceState_INITIAL;
			}

		}

		std::vector<SoundSource*>::iterator ssit = m_sourcecomponents.begin();
		for (; ssit != m_sourcecomponents.end(); ssit++) {
			(*ssit)->Update(dt);
		}

	}

	void SoundSystem::SetVolume(Float32 v) {
		m_volume = v;
		if (m_volume > 1) { m_volume = 1; }
		else if (m_volume < 0) { m_volume = 0; }
		alListenerf(AL_GAIN, m_volume);
	}

	Source* SoundSystem::RequestSource() {
		Source* src = NULL;
		for (Uint32 i = 0; i < RG_SOURCEPOOL_SIZE; i++) {
			if (m_sourcepool[i].srcstate != SourceState_PLAYING) {
				return &m_sourcepool[i];
			}
		}
		return src;
	}

}