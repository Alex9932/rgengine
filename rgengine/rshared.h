#ifndef _RSHARED_H
#define _RSHARED_H

#include "rendertypes.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern RG_DECLSPEC SDL_Window*     R_ShowWindow(Uint32 w, Uint32 h);
	extern RG_DECLSPEC void            R_Setup();
	//extern RG_DECLSPEC void        R_Initialize(SDL_Window* hwnd);
	extern RG_DECLSPEC RRenderDevice*  R_CreateDevice(RRenderSetupInfo* info);
	//extern RG_DECLSPEC void        R_Destroy();
	extern RG_DECLSPEC void            R_DestroyDevice(RRenderDevice* device);
	extern RG_DECLSPEC void            R_SwapBuffers(RRenderDevice* device, RSwapBuffersInfo* info);
	//extern RG_DECLSPEC void        R_GetInfo(RenderInfo* info);
	extern RG_DECLSPEC void            R_GetInfo(RRenderDevice* dev, RenderInfo* info);

	// ImGUI rendering backend functions
	extern RG_DECLSPEC void            R_ImGui_Init(RRenderDevice* dev);
	extern RG_DECLSPEC void            R_ImGui_Shutdown(RRenderDevice* dev);
	extern RG_DECLSPEC void            R_ImGui_NewFrame(RRenderDevice* dev);

	extern RG_DECLSPEC RBuffer*        R_CreateBuffer(RRenderDevice* dev, RBufferCreateInfo* info);
	extern RG_DECLSPEC void            R_DestroyBuffer(RBuffer* buffer);
	extern RG_DECLSPEC RImage*         R_CreateImage(RRenderDevice* dev, RImageCreateInfo* info);
	extern RG_DECLSPEC void            R_DestroyImage(RImage* image);
	extern RG_DECLSPEC RCommandBuffer* R_CreateCommandBuffer(RRenderDevice* dev, RCommandBufferCreateInfo* info);
	extern RG_DECLSPEC void            R_DestroyCommandBuffer(RCommandBuffer* buffer);
	extern RG_DECLSPEC void            R_ResetCommandBuffer(RCommandBuffer* buffer);
	extern RG_DECLSPEC void            R_BeginCommandBuffer(RCommandBuffer* buffer);
	extern RG_DECLSPEC void            R_EndCommandBuffer(RCommandBuffer* buffer);
	extern RG_DECLSPEC void            R_SubmitCommandBuffer(RCommandBufferSubmitInfo* info);
	extern RG_DECLSPEC RResourceView*  R_CreateResourceView(RRenderDevice* dev, RResourceViewCreateInfo* info);
	extern RG_DECLSPEC void            R_DestroyResourceView(RResourceView* rv);
	extern RG_DECLSPEC RRenderpass*    R_CreateRenderpass(RRenderDevice* dev, RRenderpassCreateInfo* info);
	extern RG_DECLSPEC void            R_DestroyRenderpass(RRenderpass* rp);
	extern RG_DECLSPEC RPipeline*      R_CreatePipeline(RRenderDevice* dev, RPipelineCreateInfo* info);
	extern RG_DECLSPEC void            R_DestroyPipeline(RPipeline* pl);

	extern RG_DECLSPEC void            R_CmdBeginRenderpass(RCommandBuffer* cmdbuff, RRenderpass* rp);
	extern RG_DECLSPEC void            R_CmdEndRenderpass(RCommandBuffer* cmdbuff);
	extern RG_DECLSPEC void            R_CmdBindPipeline(RCommandBuffer* cmdbuff, RPipeline* pl);
	extern RG_DECLSPEC void            R_CmdBindVertexBuffer(RCommandBuffer* cmdbuff, RBuffer* vb, Uint32 slot);
	extern RG_DECLSPEC void            R_CmdBindIndexBuffer(RCommandBuffer* cmdbuff, RBuffer* ib, IndexType isize);
	extern RG_DECLSPEC void            R_CmdDrawIndexed(RCommandBuffer* cmdbuff, Uint32 idxcount, Uint32 idxstart);

	extern RG_DECLSPEC void            R_CmdImGuiRenderDrawData(RCommandBuffer* cmdbuff, void* drawData);



	// All of this is DEPRECATED!
#if 0
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
	extern RG_DECLSPEC R2D_Texture* R2D_CreateMemTexture(R2DCreateMemTextureInfo* info);
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

	extern RG_DECLSPEC R3D_Material*       R3D_CreateMaterial(R3DCreateMaterialInfo* info);
	extern RG_DECLSPEC void                R3D_DestroyMaterial(R3D_Material* hmat);

	extern RG_DECLSPEC R3D_StaticModel*    R3D_CreateStaticModel(R3DStaticModelInfo* info);
	extern RG_DECLSPEC void				   R3D_DestroyStaticModel(R3D_StaticModel* hsmdl);

	extern RG_DECLSPEC R3D_RiggedModel*    R3D_CreateRiggedModel(R3DRiggedModelInfo* info);
	extern RG_DECLSPEC void				   R3D_DestroyRiggedModel(R3D_RiggedModel* hsmdl);

	extern RG_DECLSPEC R3D_BoneBuffer*     R3D_CreateBoneBuffer(R3DCreateBufferInfo* info);
	extern RG_DECLSPEC void				   R3D_DestroyBoneBuffer(R3D_BoneBuffer* hbuff);
	extern RG_DECLSPEC void                R3D_UpdateBoneBuffer(R3DUpdateBufferInfo* info);

	extern RG_DECLSPEC R3D_AtlasHandle*    R3D_CreateAtlas(String texture);
	extern RG_DECLSPEC void                R3D_DestroyAtlas(R3D_AtlasHandle* atlas);

	extern RG_DECLSPEC R3D_ParticleBuffer* R3D_CreateParticleBuffer(R3DCreateBufferInfo* info);
	extern RG_DECLSPEC void                R3D_DestroyParticleBuffer(R3D_ParticleBuffer* hbuff);
	extern RG_DECLSPEC void                R3D_UpdateParticleBuffer(R3DUpdateBufferInfo* info);

	extern RG_DECLSPEC void                R3D_PushLightSource(R3D_LightSource* info);

	extern RG_DECLSPEC void                R3D_PushModel(R3D_PushModelInfo* info);
	extern RG_DECLSPEC void				   R3D_SetCamera(R3D_CameraInfo* info);

	extern RG_DECLSPEC void				   R3D_StartRenderTask(R3D_RenderTaskInfo* info);
#endif

#ifdef __cplusplus
}
#endif

#endif