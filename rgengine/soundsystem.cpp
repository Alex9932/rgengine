#define DLL_EXPORT
#include "soundsystem.h"
#include "engine.h"

#define RG_SOURCEPOOL_SIZE 128

namespace Engine {

	struct Source {
		ALuint source;
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
		m_format = GetFormat(info->channels);
		alGenBuffers(1, &m_buffer);
		alBufferData(m_buffer, m_format, info->data, info->samples * info->channels, info->samplerate);
	}

	SoundBuffer::~SoundBuffer() {
		alDeleteBuffers(1, &m_buffer);
	}

	// Stream buffer
	StreamBuffer::StreamBuffer() {
	}

	StreamBuffer::~StreamBuffer() {
	}

	// Sound system
	SoundSystem::SoundSystem() {
		rgLogInfo(RG_LOG_SYSTEM, "Starting up sound system...");
		m_sourcepool = RG_NEW(PoolAllocator)("Sound pool", RG_SOURCEPOOL_SIZE, sizeof(Source));

		m_device = alcOpenDevice(NULL);
		m_ctx    = alcCreateContext(m_device, NULL);
		if (!alcMakeContextCurrent(m_ctx)) {
			RG_ERROR_MSG("Failed to create ALC context!");
		}

		// Make sources
		Source* sources = (Source*)m_sourcepool->GetBasePointer();
		for (Uint32 i = 0; i < RG_SOURCEPOOL_SIZE; i++) {
			alGenSources(1, &sources[i].source);
			sources[i].buffer = NULL;
		}

	}

	SoundSystem::~SoundSystem() {

		Source* sources = (Source*)m_sourcepool->GetBasePointer();
		for (Uint32 i = 0; i < RG_SOURCEPOOL_SIZE; i++) {
			alDeleteSources(1, &sources[i].source);
		}
		m_sourcepool->DeallocateAll();

		RG_DELETE(PoolAllocator, m_sourcepool);
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
		Source* src = (Source*)m_sourcepool->Allocate();
		if (!src) {
			rgLogError(RG_LOG_SYSTEM, "No sound source available.");
		}

		src->buffer = info->buffer;

		alSourcei(src->source,  AL_BUFFER,   src->buffer->GetBuffer());

		alSourcef(src->source,  AL_PITCH,    info->speed);
		alSourcef(src->source,  AL_GAIN,     info->volume);
		alSourcei(src->source,  AL_LOOPING,  AL_FALSE);
		alSourcefv(src->source, AL_POSITION, info->position.array);
		alSource3f(src->source, AL_VELOCITY, 0, 0, 0);

		alSourcePlay(src->source);
		
	}

	void SoundSystem::Update(Float64 dt) {

		Source* sources = (Source*)m_sourcepool->GetBasePointer();
		ALint ival = 0;
		for (Uint32 i = 0; i < RG_SOURCEPOOL_SIZE; i++) {
			Source* src = &sources[i];
			// Get state
			alGetSourcei(src->source, AL_SOURCE_STATE, &ival);
			if (ival == AL_STOPPED) {
				// Free source if play stopped
				m_sourcepool->Deallocate(src);
			}

		}
	}

}