#ifndef _SOUNDBUFFER_H
#define _SOUNDBUFFER_H

#include "rgtypes.h"
#include "rgstb.h"
#include <AL/al.h>

#define RG_SOUND_LOOPING 0x00000001
#define RG_SOUND_PAUSED  0x00000002
#define RG_SOUND_ENDED   0x00000004
#define RG_SOUND_PLAYING 0x00000008
/* RESERVED
#define RG_SOUND_ 0x00000010
#define RG_SOUND_ 0x00000020
#define RG_SOUND_ 0x00000040
#define RG_SOUND_ 0x00000080
*/

namespace Engine {

	class SoundSource;

	typedef struct SoundBufferCreateInfo {
		Uint32 samplerate;
		Uint32 channels;
		Uint32 samples;
		void* data;
	} SoundBufferCreateInfo;

	enum SoundBufferType {
		SBType_STATIC = 0,
		SBType_STREAM
	};

	class ISoundBuffer {
		public:
			ISoundBuffer() {}
			~ISoundBuffer() {}
			virtual void Update() {}
			virtual void Play() {}
			virtual void Stop() {}
			virtual void Pause() {}
			RG_INLINE SoundBufferType GetBufferType() { return m_type; }
			RG_INLINE SoundSource* GetSource() { return m_source; }

			RG_INLINE void SetSource(SoundSource* src) { m_source = src; }

		protected:
			SoundBufferType m_type = SBType_STATIC;
			SoundSource* m_source = NULL;
	};

	class SoundBuffer : public ISoundBuffer {
		public:
			SoundBuffer(SoundBufferCreateInfo* info);
			~SoundBuffer();
			virtual void Update() {}
			RG_INLINE ALuint GetBuffer() { return m_buffer; }

		private:
			ALuint m_buffer = 0;
			ALenum m_format = 0;
	};

	class StreamBuffer : public ISoundBuffer {
		public:
			RG_DECLSPEC StreamBuffer(String stream);
			RG_DECLSPEC ~StreamBuffer();
			virtual void Update();
			virtual void Play();
			virtual void Stop();
			virtual void Pause();

		private:
			RG_STB_VORBIS   m_stream;
			stb_vorbis_info m_info;
			ALuint          m_buffers[2];
			ALenum          m_format = 0;
			Uint32          m_flags = 0;
			Uint32          m_current = 0;
	};
}

#endif