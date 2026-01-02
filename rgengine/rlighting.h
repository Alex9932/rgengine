#ifndef _RLIGHTING_H
#define _RLIGHTING_H

union ivec2;

namespace Engine {
	namespace Render {

		void InitRLighting(ivec2* size);
		void DestroyRLighting();
		void ResizeRLighting(ivec2* size);
		void* GetRLightingOutputSet();
		void DoRLighting();

	}
}

#endif
