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

	// R2D

	extern RG_DECLSPEC R2D_Buffer*  R2D_CreateBuffer(R2DCreateBufferInfo* info);
	extern RG_DECLSPEC void         R2D_DestroyBuffer(R2D_Buffer* buffer);
	extern RG_DECLSPEC void         R2D_BufferData(R2DBufferDataInfo* info);

	extern RG_DECLSPEC R2D_Texture* R2D_CreateTexture(R2DCreateTextureInfo* info);
	extern RG_DECLSPEC void         R2D_DestroyTexture(R2D_Texture* texture);
	extern RG_DECLSPEC void         R2D_TextureData(R2DTextureDataInfo* info);

	extern RG_DECLSPEC void         R2D_PushMatrix(mat4* matrix);
	extern RG_DECLSPEC mat4*        R2D_PopMatrix();
	extern RG_DECLSPEC void         R2D_ResetStack();

	extern RG_DECLSPEC void         R2D_Begin();
	extern RG_DECLSPEC void         R2D_Bind(R2DBindInfo* info);
	extern RG_DECLSPEC void         R2D_Draw(R2DDrawInfo* info);

	// R3D

	//extern RG_DECLSPEC void        R_SetInfo(RenderInfo* info);

	extern RG_DECLSPEC R3D_Material*    R3D_CreateMaterial(R3DCreateMaterialInfo* info);
	extern RG_DECLSPEC void             R3D_DestroyMaterial(R3D_Material* hmat);

	extern RG_DECLSPEC R3D_StaticModel* R3D_CreateStaticModel(R3DStaticModelInfo* info);
	extern RG_DECLSPEC void				R3D_DestroyStaticModel(R3D_StaticModel* hsmdl);

	extern RG_DECLSPEC R3D_RiggedModel* R3D_CreateRiggedModel(R3DRiggedModelInfo* info);
	extern RG_DECLSPEC void				R3D_DestroyRiggedModel(R3D_RiggedModel* hsmdl);

	extern RG_DECLSPEC R3D_BoneBuffer*  R3D_CreateBoneBuffer(R3DCreateBoneBufferInfo* info);
	extern RG_DECLSPEC void				R3D_DestroyBoneBuffer(R3D_BoneBuffer* hbuff);
	extern RG_DECLSPEC void             R3D_UpdateBoneBuffer(R3DBoneBufferUpdateInfo* info);

	extern RG_DECLSPEC void				R3D_PushModel(R3D_PushModelInfo* info);
	extern RG_DECLSPEC void				R3D_SetCamera(R3D_CameraInfo* info);

	extern RG_DECLSPEC void				R3D_StartRenderTask(R3D_RenderTaskInfo* info);

#ifdef __cplusplus
}
#endif

#endif