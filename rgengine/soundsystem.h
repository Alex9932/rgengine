#ifndef _SOUNDSYSTEM_H
#define _SOUNDSYSTEM_H

#include "allocator.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <vector>

#include "soundbuffer.h"
#include "soundsource.h"

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

			Float32 GetVolume() { return m_volume; }
			RG_DECLSPEC void SetVolume(Float32 v);

		private:
			Engine::PoolAllocator*    m_alloc;
			std::vector<SoundSource*> m_sourcecomponents;
			Source*        m_sourcepool = NULL;
			ALCdevice*     m_device     = NULL;
			ALCcontext*    m_ctx        = NULL;

			Float32        m_volume     = 0.1f;

			Source* RequestSource();

	};

}

#endif