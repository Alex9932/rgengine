#ifndef _RSHARED_H
#define _RSHARED_H

#include "rgtypes.h"
#include "rendertypes.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern RG_DECLSPEC SDL_Window* R_ShowWindow(Uint32 w, Uint32 h);
	extern RG_DECLSPEC void        R_Setup(RenderSetupInfo* info);
	extern RG_DECLSPEC void        R_Initialize(SDL_Window* hwnd);
	extern RG_DECLSPEC void        R_Destroy();
	extern RG_DECLSPEC void        R_SwapBuffers();
	extern RG_DECLSPEC void        R_GetInfo(RenderInfo* info);

	//extern RG_DECLSPEC void        R_SetInfo(RenderInfo* info);

	extern RG_DECLSPEC R3D_Material*    R3D_CreateMaterial(R3DCreateMaterialInfo* info);
	extern RG_DECLSPEC void             R3D_DestroyMaterial(R3D_Material* hmat);

	extern RG_DECLSPEC R3D_StaticModel* R3D_CreateStaticModel(R3DCreateStaticModelInfo* info);
	extern RG_DECLSPEC void				R3D_DestroyStaticModel(R3D_StaticModel* hsmdl);

	extern RG_DECLSPEC R3D_RiggedModel* R3D_CreateRiggedModel(R3DCreateRiggedModelInfo* info);
	extern RG_DECLSPEC void				R3D_DestroyRiggedModel(R3D_RiggedModel* hsmdl);

	extern RG_DECLSPEC R3D_BoneBuffer*  R3D_CreateBoneBuffer(R3DCreateBoneBufferInfo* info);
	extern RG_DECLSPEC void				R3D_DestroyBoneBuffer(R3D_BoneBuffer* hbuff);
	extern RG_DECLSPEC void             R3D_UpdateBoneBuffer(R3DBoneBufferUpdateInfo* info);

	extern RG_DECLSPEC void				R3D_PushModel(R3D_PushModelInfo* info);
	extern RG_DECLSPEC void				R3D_SetCamera(R3D_CameraInfo* info);

	extern RG_DECLSPEC void				R3D_StartRenderTask(void* TODO);

#ifdef __cplusplus
}
#endif

#endif