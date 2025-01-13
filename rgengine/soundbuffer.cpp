#include "soundbuffer.h"
#include "soundsource.h"

#define RG_SND_BUFFER_SIZE 8192

namespace Engine {

	// TODO: Rewrite this !!!
	static Uint16 __data[RG_SND_BUFFER_SIZE];

	static ALenum GetFormat(Uint32 c) {
		switch (c) {
		case 1:  return AL_FORMAT_MONO16;
		case 2:  return AL_FORMAT_STEREO16;
		default: return AL_NONE;
		}
	}

	static Bool RefillBufferData(ALuint buffer, stb_vorbis* stream) {
		stb_vorbis_info info;
		RG_STB_vorbis_get_info_ptr(stream, &info);
		Uint32 channels = info.channels;
		Sint32 amount = RG_STB_vorbis_get_samples_short_interleaved(stream, channels, (short*)__data, RG_SND_BUFFER_SIZE);

		if (amount <= 0) {
			return false; // Stream ended
		}

		alBufferData(buffer, GetFormat(channels), __data, amount * channels * sizeof(short), info.sample_rate);
		return true;
	}

	/////////////////////////////////////////////////////
	// Sound buffer
	/////////////////////////////////////////////////////
	SoundBuffer::SoundBuffer(SoundBufferCreateInfo* info) {
		m_type = SBType_STATIC;
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
	StreamBuffer::StreamBuffer(String stream) {
		m_type = SBType_STREAM;
		m_stream = RG_STB_vorbis_open_file(stream, NULL, NULL);
		alGenBuffers(2, m_buffers);

		//m_info = RG_STB_vorbis_get_info(stream);

	}

	StreamBuffer::~StreamBuffer() {
		alDeleteBuffers(2, m_buffers);
		RG_STB_vorbis_close(&m_stream);
	}

	void StreamBuffer::Update() {
		ALint  proc;
		ALuint buff;

		alGetSourcei(m_source->GetSource(), AL_BUFFERS_PROCESSED, &proc);
		if (!RG_CHECK_FLAG(m_flags, RG_SOUND_PAUSED) &&
			!RG_CHECK_FLAG(m_flags, RG_SOUND_ENDED) &&
			proc) {

			rgLogInfo(RG_LOG_DEBUG, "Process buffer");

			alSourceUnqueueBuffers(m_source->GetSource(), 1, &buff);
			//m_current = (m_current++) & 1;

			if (!RefillBufferData(buff, m_stream.stream)) {
				if (m_source->IsLooping()) {
					RG_STB_vorbis_seek_start(m_stream.stream);
					RefillBufferData(buff, m_stream.stream);
				}
				else {
					RG_SET_FLAG(m_flags, RG_SOUND_ENDED);
				}
			}

			alSourceQueueBuffers(m_source->GetSource(), 1, &buff);
		}
		if (proc == 2) {
			alSourcePlay(m_source->GetSource());
		}

	}

	void StreamBuffer::Play() {
		if (!RG_CHECK_FLAG(m_flags, RG_SOUND_PAUSED)) {

			rgLogInfo(RG_LOG_DEBUG, "Initial queue buffers");
			// Queue 2 buffers
			m_current = 0;
			RefillBufferData(m_buffers[m_current], m_stream.stream);
			alSourceQueueBuffers(m_source->GetSource(), 1, &m_buffers[m_current]);
			m_current = (m_current++) & 1;

			RefillBufferData(m_buffers[m_current], m_stream.stream);
			alSourceQueueBuffers(m_source->GetSource(), 1, &m_buffers[m_current]);
			m_current = (m_current++) & 1;

		}
		else {
			rgLogInfo(RG_LOG_DEBUG, "RESUME");
		}
		RG_RESET_FLAG(m_flags, RG_SOUND_PAUSED);
		RG_RESET_FLAG(m_flags, RG_SOUND_ENDED);
		RG_SET_FLAG(m_flags, RG_SOUND_PLAYING);
	}

	void StreamBuffer::Stop() {
		RG_RESET_FLAG(m_flags, RG_SOUND_PLAYING);
		RG_RESET_FLAG(m_flags, RG_SOUND_PAUSED);
		RG_SET_FLAG(m_flags, RG_SOUND_ENDED);

		ALint  proc;
		ALuint buff[2];
		alGetSourcei(m_source->GetSource(), AL_BUFFERS_PROCESSED, &proc);
		if (proc != 0) {
			alSourceUnqueueBuffers(m_source->GetSource(), proc, buff);
		}

		RG_STB_vorbis_seek_start(m_stream.stream);

	}

	void StreamBuffer::Pause() {
		RG_SET_FLAG(m_flags, RG_SOUND_PAUSED);
		RG_RESET_FLAG(m_flags, RG_SOUND_PLAYING);
	}

}