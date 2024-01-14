#ifndef _SOUNDSYSTEM_H
#define _SOUNDSYSTEM_H

#include "entity.h"
#include "allocator.h"

#include "rgstb.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <vector>

/*

Objects:
 - Listener
 - SoundSource
 - Buffer
 - StreamBuffer

Functions:
 - Playsound ( Buffer sound, vec3 position )
 - Source.Playstream ( StreamBuffer stream )

*/

#define RG_SOUND_LOOPING 0x00000001
#define RG_SOUND_PAUSED  0x00000002
#define RG_SOUND_ENDED   0x00000004
/* RESERVED
#define RG_SOUND_ 0x00000008
#define RG_SOUND_ 0x00000010
#define RG_SOUND_ 0x00000020
#define RG_SOUND_ 0x00000040
#define RG_SOUND_ 0x00000080
*/

namespace Engine {

	typedef struct SoundBufferCreateInfo {
		Uint32 samplerate;
		Uint32 channels;
		Uint32 samples;
		void*  data;
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
			RG_INLINE SoundBufferType GetBufferType()  { return m_type;   }
			RG_INLINE SoundSource*    GetSource()      { return m_source; }

			RG_INLINE void SetSource(SoundSource* src) { m_source = src;  }

		protected:
			SoundBufferType m_type   = SBType_STATIC;
			SoundSource*    m_source = NULL;
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
			RG_DECLSPEC StreamBuffer(stb_vorbis* stream);
			RG_DECLSPEC ~StreamBuffer();
			virtual void Update();
			virtual void Play();
			virtual void Stop();
			virtual void Pause();

		private:
			stb_vorbis*     m_stream;
			stb_vorbis_info m_info;
			ALuint          m_buffers[2];
			ALenum          m_format  = 0;
			Uint32          m_flags   = 0;
			Uint32          m_current = 0;

	};

	// Source for SoundBuffer & StreamBuffer
	class SoundSource : public Component {
		public:
			SoundSource();
			~SoundSource();

			virtual void Destroy();
			void Update(Float64 dt);

			RG_DECLSPEC void SetBuffer(ISoundBuffer* buffer);

			RG_DECLSPEC void Play();
			RG_DECLSPEC void Stop();
			RG_DECLSPEC void Pause();

			RG_DECLSPEC void SetRepeat(Bool repeat);

			RG_DECLSPEC Bool IsPaused();
			RG_DECLSPEC Bool IsEnded();
			RG_DECLSPEC Bool IsLooping();

			RG_INLINE ALuint GetSource() { return m_source; }

		private:
			ISoundBuffer* m_buffer = NULL;
			ALuint        m_source = 0;
			Uint32        m_flags  = 0;

	};

	typedef struct Source;
	typedef struct PlaySoundInfo {
		SoundBuffer* buffer;
		vec3         position;
		Float32      volume;
		Float32      speed;
	} PlaySoundInfo;

	class SoundSystem {
		public:
			SoundSystem();
			~SoundSystem();

			RG_DECLSPEC SoundBuffer* CreateBuffer(SoundBufferCreateInfo* info);
			RG_DECLSPEC void DestroyBuffer(SoundBuffer* ptr);

			RG_DECLSPEC SoundSource* NewSoundSource();
			RG_DECLSPEC void DeleteSoundSource(SoundSource* comp);

			RG_DECLSPEC void PlaySound(PlaySoundInfo* info);

			void Update(Float64 dt);

		private:
			Engine::PoolAllocator*    m_alloc;
			std::vector<SoundSource*> m_sourcecomponents;
			Source*        m_sourcepool = NULL;
			ALCdevice*     m_device     = NULL;
			ALCcontext*    m_ctx        = NULL;

			Source* RequestSource();

	};

}

#endif