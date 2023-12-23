#ifndef _RENDER_H
#define _RENDER_H

#include "rgtypes.h"
#include "rendertypes.h"

#define RG_EVENT_RENDER_VIEWPORT_RESIZE 0x00010001

// Render core
typedef SDL_Window*       (*PFN_R_SHOWWINDOW)(Uint32, Uint32); // Width, height
typedef void              (*PFN_R_SETUP)(RenderSetupInfo*);
typedef void              (*PFN_R_INITIALIZE)(SDL_Window*);
typedef void              (*PFN_R_DESTROY)();
typedef void              (*PFN_R_SWAPBUFFERS)();
typedef void              (*PFN_R_GETINFO)(RenderInfo*);

typedef R3D_Material*     (*PFN_R3D_CREATEMATERIAL)(R3DCreateMaterialInfo*);
typedef void              (*PFN_R3D_DESTROYMATERIAL)(R3D_Material*);

typedef R3D_StaticModel*  (*PFN_R3D_CREATESTATICMODEL)(R3DStaticModelInfo*);
typedef void              (*PFN_R3D_DESTROYSTATICMODEL)(R3D_StaticModel*);

typedef R3D_RiggedModel*  (*PFN_R3D_CREATERIGGEDMODEL)(R3DRiggedModelInfo*);
typedef void              (*PFN_R3D_DESTROYRIGGEDMODEL)(R3D_RiggedModel*);

typedef R3D_BoneBuffer*   (*PFN_R3D_CREATEBONEBUFFER)(R3DCreateBoneBufferInfo*);
typedef void              (*PFN_R3D_DESTROYBONEBUFFER)(R3D_BoneBuffer*);
typedef void              (*PFN_R3D_UPDATEBONEBUFFER)(R3DBoneBufferUpdateInfo*);

typedef void              (*PFN_R3D_PUSHMODEL)(R3D_PushModelInfo*);
typedef void              (*PFN_R3D_SETCAMERA)(R3D_CameraInfo*);

typedef void			  (*PFN_R3D_STARTRENDERTASK)(void*);

namespace Engine {

	class ModelSystem;
	class LightSystem;

	namespace Render {

		// Core
		extern RG_DECLSPEC PFN_R_SHOWWINDOW            ShowWindow;
		extern RG_DECLSPEC PFN_R_SETUP                 Setup;
		extern RG_DECLSPEC PFN_R_INITIALIZE            Initialize;
		extern RG_DECLSPEC PFN_R_DESTROY               Destroy;
		extern RG_DECLSPEC PFN_R_SWAPBUFFERS           SwapBuffers;
		extern RG_DECLSPEC PFN_R_GETINFO               GetInfo;

		// R3D
		extern RG_DECLSPEC PFN_R3D_CREATEMATERIAL      R3D_CreateMaterial;
		extern RG_DECLSPEC PFN_R3D_DESTROYMATERIAL     R3D_DestroyMaterial;

		extern RG_DECLSPEC PFN_R3D_CREATESTATICMODEL   R3D_CreateStaticModel;
		extern RG_DECLSPEC PFN_R3D_DESTROYSTATICMODEL  R3D_DestroyStaticModel;

		extern RG_DECLSPEC PFN_R3D_CREATERIGGEDMODEL   R3D_CreateRiggedModel;
		extern RG_DECLSPEC PFN_R3D_DESTROYRIGGEDMODEL  R3D_DestroyRiggedModel;

		extern RG_DECLSPEC PFN_R3D_CREATEBONEBUFFER    R3D_CreateBoneBuffer;
		extern RG_DECLSPEC PFN_R3D_DESTROYBONEBUFFER   R3D_DestroyBoneBuffer;
		extern RG_DECLSPEC PFN_R3D_UPDATEBONEBUFFER    R3D_UpdateBoneBuffer;

		extern RG_DECLSPEC PFN_R3D_PUSHMODEL           R3D_PushModel;
		extern RG_DECLSPEC PFN_R3D_SETCAMERA           R3D_SetCamera;

		RG_DECLSPEC void          LoadRenderer(String path);
		RG_DECLSPEC void          UnloadRenderer();
		RG_DECLSPEC Bool          IsRendererLoaded();
		RG_DECLSPEC LibraryHandle GetHandle();

		void InitSubSystem();
		void DestroySubSystem();

		RG_DECLSPEC void Update();
		//RG_DECLSPEC void ToggleConsole();

		RG_DECLSPEC ModelSystem* GetModelSystem();
		RG_DECLSPEC LightSystem* GetLightSystem();

		RG_DECLSPEC R3D_BoneBuffer* CreateBoneBuffer(R3DCreateBoneBufferInfo* info);

		RG_DECLSPEC RenderSetupInfo* GetSetupParams();
		RG_DECLSPEC void SetRenderFlags(RenderFlags flags);

	}
}

#endif