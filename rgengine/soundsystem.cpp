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

	static Bool RefillBufferData(ALuint buffer, stb_vorbis* stream) {
		Uint16 data[65536];
		stb_vorbis_info info;
		RG_STB_vorbis_get_info_ptr(stream, &info);
		Uint32 channels = info.channels;
		Sint32 amount = RG_STB_vorbis_get_samples_short_interleaved(stream, channels, (short*)data, 65536);

		if (amount <= 0) {
			return false; // Stream ended
		}

		alBufferData(buffer, GetFormat(channels), data, amount * channels * sizeof(short), info.sample_rate);
		return true;
	}

	
	/////////////////////////////////////////////////////
	// Sound source component
	/////////////////////////////////////////////////////
	SoundSource::SoundSource() : Component(Component_SOUNDSOURCE) {
		alGenSources(1, &m_source);
	}

	SoundSource::~SoundSource() { Destroy(); }

	void SoundSource::Destroy() {
		alDeleteSources(1, &m_source);
	}

	void SoundSource::Update(Float64 dt) {
		if (m_buffer != NULL) {
			m_buffer->Update();
		}
	}

	void SoundSource::SetBuffer(ISoundBuffer* buffer) {
		if (m_buffer != NULL) {
			m_buffer->SetSource(NULL);
		}
		m_buffer = buffer;
		if (buffer != NULL) {
			buffer->SetSource(this);
		}
	}

	void SoundSource::Play() {
		if (m_buffer == NULL) { return; }
		m_buffer->Play();
		alSourcePlay(m_source);
	}

	void SoundSource::Stop() {
		if (m_buffer == NULL) { return; }
		m_buffer->Stop();
		alSourceStop(m_source);
		//alSourceUnqueueBuffers();
	}

	void SoundSource::Pause() {
		if (m_buffer == NULL) { return; }
		m_buffer->Pause();
		alSourcePause(m_source);
	}

	void SoundSource::SetRepeat(Bool repeat) {
		if (repeat) {
			RG_SET_FLAG(m_flags, RG_SOUND_LOOPING);
		} else {
			RG_RESET_FLAG(m_flags, RG_SOUND_LOOPING);
		}
	}

	Bool SoundSource::IsPaused() {
		if (m_buffer == NULL) { return false; }
		return RG_CHECK_FLAG(m_flags, RG_SOUND_PAUSED);
	}

	Bool SoundSource::IsEnded() {
		if (m_buffer == NULL) { return true; }
		return RG_CHECK_FLAG(m_flags, RG_SOUND_ENDED);
	}

	Bool SoundSource::IsLooping() {
		if (m_buffer == NULL) { return false; }
		return RG_CHECK_FLAG(m_flags, RG_SOUND_LOOPING);
	}


	/////////////////////////////////////////////////////
	// Sound buffer
	/////////////////////////////////////////////////////
	SoundBuffer::SoundBuffer(SoundBufferCreateInfo* info) {
		m_type   = SBType_STATIC;
		m_format = GetFormat(info->channels);
		alGenBuffers(1, &m_buffer);
		alBufferData(m_buffer, m_format, info->data, info->samples * info->channels, info->samplerate);
	}

	SoundBuffer::~SoundBuffer() {
		alDeleteBuffers(1, &m_buffer);
	}


	/////////////////////////////////////////////////////
	// Stream buffer
	/////////////////////////////////////////////////////
	StreamBuffer::StreamBuffer(stb_vorbis* stream) {
		m_type   = SBType_STREAM;
		m_stream = stream;
		alGenBuffers(2, m_buffers);

		//m_info = RG_STB_vorbis_get_info(stream);

	}

	StreamBuffer::~StreamBuffer() {
		alDeleteBuffers(2, m_buffers);
	}

	void StreamBuffer::Update() {
		ALint  proc;
		ALuint buff;

		alGetSourcei(m_source->GetSource(), AL_BUFFERS_PROCESSED, &proc);
		if (!RG_CHECK_FLAG(m_flags, RG_SOUND_PAUSED) &&
			!RG_CHECK_FLAG(m_flags, RG_SOUND_ENDED) &&
			proc) {

			rgLogInfo(RG_LOG_SYSTEM, "Process buffer");

			alSourceUnqueueBuffers(m_source->GetSource(), 1, &buff);
			m_current = (m_current++) & 1;

			if (!RefillBufferData(buff, m_stream)) {
				if (m_source->IsLooping()) {
					RG_STB_vorbis_seek_start(m_stream);
					RefillBufferData(buff, m_stream);
				} else {
					RG_SET_FLAG(m_flags, RG_SOUND_ENDED);
				}
			}

			alSourceQueueBuffers(m_source->GetSource(), 1, &buff);
		}

	}

	void StreamBuffer::Play() {
		if(!RG_CHECK_FLAG(m_flags, RG_SOUND_PAUSED)) {

			rgLogInfo(RG_LOG_SYSTEM, "Initial queue buffers");
			// Queue 2 buffers
			RefillBufferData(m_buffers[m_current], m_stream);
			alSourceQueueBuffers(m_source->GetSource(), 1, &m_buffers[m_current]);
			m_current = (m_current++) & 1;

			RefillBufferData(m_buffers[m_current], m_stream);
			alSourceQueueBuffers(m_source->GetSource(), 1, &m_buffers[m_current]);
			m_current = (m_current++) & 1;

		}
		RG_RESET_FLAG(m_flags, RG_SOUND_PAUSED);
		RG_RESET_FLAG(m_flags, RG_SOUND_ENDED);
	}

	void StreamBuffer::Stop() {
		RG_RESET_FLAG(m_flags, RG_SOUND_PAUSED);
		RG_SET_FLAG(m_flags, RG_SOUND_ENDED);
	}

	void StreamBuffer::Pause() {
		RG_SET_FLAG(m_flags, RG_SOUND_PAUSED);
	}


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
			if (*it = comp) {
				m_sourcecomponents.erase(it);
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

		std::vector<SoundSource*>::iterator ssit = m_sourcecomponents.begin();
		for (; ssit != m_sourcecomponents.end(); ssit++) {
			(*ssit)->Update(dt);
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