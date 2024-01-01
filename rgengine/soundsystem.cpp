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

	static ALenum GetFormat(Uint32 c) {
		switch (c) {
			case 1:  return AL_FORMAT_MONO16;
			case 2:  return AL_FORMAT_STEREO16;
			default: return AL_NONE;
		}
	}
	
	// Sound source component
	SoundSource::SoundSource() : Component(Component_SOUNDSOURCE) {

	}

	SoundSource::~SoundSource() {

	}

	void SoundSource::Destroy() {

	}

	void SoundSource::Update(Float64 dt) {

	}

	// Sound buffer
	SoundBuffer::SoundBuffer(SoundBufferCreateInfo* info) {
		m_type   = SBType_STATIC;
		m_format = GetFormat(info->channels);
		alGenBuffers(1, &m_buffer);
		alBufferData(m_buffer, m_format, info->data, info->samples * info->channels, info->samplerate);
	}

	SoundBuffer::~SoundBuffer() {
		alDeleteBuffers(1, &m_buffer);
	}

	// Stream buffer
	StreamBuffer::StreamBuffer() {
		m_type = SBType_STREAM;
	}

	StreamBuffer::~StreamBuffer() {
	}

	// Sound system
	SoundSystem::SoundSystem() {
		rgLogInfo(RG_LOG_SYSTEM, "Starting up sound system...");
		m_sourcepool = (Source*)GetDefaultAllocator()->Allocate(sizeof(Source) * RG_SOURCEPOOL_SIZE);

		m_device = alcOpenDevice(NULL);
		m_ctx    = alcCreateContext(m_device, NULL);
		if (!alcMakeContextCurrent(m_ctx)) {
			RG_ERROR_MSG("Failed to create ALC context!");
		}

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

		alcDestroyContext(m_ctx);
		alcCloseDevice(m_device);
	}

	SoundBuffer* SoundSystem::CreateBuffer(SoundBufferCreateInfo* info) {
		return new SoundBuffer(info); // TMP
	}

	void SoundSystem::DestroyBuffer(SoundBuffer* ptr) {
		delete ptr; // TMP
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

	void SoundSystem::Update(Float64 dt) {

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