#ifndef _SOUNDSOURCE_H
#define _SOUNDSOURCE_H

#include "entity.h"
#include <AL/al.h>

namespace Engine {

	class ISoundBuffer;

	// Source for SoundBuffer & StreamBuffer
	class SoundSource : public Component {
		public:
			SoundSource();
			~SoundSource();

			virtual void Destroy();
			void Update(Float64 dt);

			void SetPosition(const vec3& pos);

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
			Uint32        m_flags = 0;

	};

}

#endif