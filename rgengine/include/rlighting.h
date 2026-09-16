#ifndef _RLIGHTING_H
#define _RLIGHTING_H

union ivec2;
struct R3D_LightSource;
struct RImage;
struct RDescriptorSet;

namespace Engine {
	namespace Render {

		void InitRLighting(ivec2* size);
		void DestroyRLighting();
		void ResizeRLighting(ivec2* size);
		void ReloadRLighting(ivec2* size);
		void PushSourceRLighting(R3D_LightSource* light);
		void DoRLighting();

		RImage* GetRLightingOutput();
		RDescriptorSet* GetRLightingOutputSet();

	}
}

#endif
