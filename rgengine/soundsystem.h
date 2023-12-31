#ifndef _SOUNDSYSTEM_H
#define _SOUNDSYSTEM_H

#include "entity.h"
#include "allocator.h"

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

namespace Engine {

	typedef struct SoundBufferCreateInfo {
		Uint32 samplerate;
		Uint32 channels;
		Uint32 samples;
		void*  data;
	} SoundBufferCreateInfo;

	// Source for SoundBuffer & StreamBuffer
	class SoundSource : public Component {
		public:
			SoundSource();
			~SoundSource();

			void Destroy();
			void Update(Float64 dt);

		private:

	};

	class SoundBuffer {
		public:
			SoundBuffer(SoundBufferCreateInfo* info);
			~SoundBuffer();
			RG_INLINE ALuint GetBuffer() { return m_buffer; }

		private:
			ALuint m_buffer = 0;
			ALenum m_format = 0;
	};

	class StreamBuffer {
		public:
			StreamBuffer();
			~StreamBuffer();

		private:

	};



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

			RG_DECLSPEC void PlaySound(PlaySoundInfo* info);

			void Update(Float64 dt);

		private:
			std::vector<SoundSource*> m_sourcecomponents;
			PoolAllocator* m_sourcepool = NULL;

			ALCdevice*     m_device     = NULL;
			ALCcontext*    m_ctx        = NULL;

	};

}

#endif