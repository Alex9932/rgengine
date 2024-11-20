#ifndef _RENDER_H
#define _RENDER_H

#include "rgtypes.h"
#include "rendertypes.h"

#include "renderctx.h"

#define RG_EVENT_RENDER_VIEWPORT_RESIZE 0x00010001

namespace Engine {

	class ModelSystem;
	class LightSystem;
	class ParticleSystem;

	namespace Render {

		RG_DECLSPEC void          LoadRenderer(String path);
		RG_DECLSPEC void          UnloadRenderer();
		RG_DECLSPEC Bool          IsRendererLoaded();
		RG_DECLSPEC LibraryHandle GetHandle();

		void InitSubSystem();
		void DestroySubSystem();

		SDL_Window* ShowWindow(Uint32 w, Uint32 h);
		void InitializeContext(SDL_Window* hwnd);
		void SwapBuffers();

		// ImGui window
		RG_DECLSPEC void DrawRendererStats();
		RG_DECLSPEC void DrawProfilerStats();

		RG_DECLSPEC void SetCamera(R3D_CameraInfo* info);

		RG_DECLSPEC void UpdateSystems();
		RG_DECLSPEC void Update();

		RG_DECLSPEC void SetGlobalLight(R3D_GlobalLightDescrition* desc);
		RG_DECLSPEC void GetInfo(RenderInfo* info);

		//RG_DECLSPEC void ToggleConsole();

		RG_DECLSPEC ModelSystem* GetModelSystem();
		RG_DECLSPEC LightSystem* GetLightSystem();
		RG_DECLSPEC ParticleSystem* GetParticleSystem();


		RG_DECLSPEC R3D_StaticModel* CreateStaticModel(R3DStaticModelInfo* info);
		RG_DECLSPEC void DestroyStaticModel(R3D_StaticModel* mdl);

		RG_DECLSPEC R3D_RiggedModel* CreateRiggedModel(R3DRiggedModelInfo* info);
		RG_DECLSPEC void DestroyRiggedModel(R3D_RiggedModel* mdl);

		RG_DECLSPEC R3D_BoneBuffer* CreateBoneBuffer(R3DCreateBufferInfo* info);
		RG_DECLSPEC void DestroyBoneBuffer(R3D_BoneBuffer* hbuff);
		RG_DECLSPEC void UpdateBoneBuffer(R3DUpdateBufferInfo* info);

		RG_DECLSPEC R3D_AtlasHandle* CreateAtlas(String texture);
		RG_DECLSPEC void DestroyAtlas(R3D_AtlasHandle* atlas);

		RG_DECLSPEC R3D_ParticleBuffer* CreateParticleBuffer(R3DCreateBufferInfo* info);
		RG_DECLSPEC void DestroyParticleBuffer(R3D_ParticleBuffer* hbuff);
		RG_DECLSPEC void UpdateParticleBuffer(R3DUpdateBufferInfo* info);

		RG_DECLSPEC RenderSetupInfo* GetSetupParams();
		RG_DECLSPEC void SetRenderFlags(Uint32 flags);

	}
}

#endif