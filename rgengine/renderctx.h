/*
 * rgEngine renderctx.h
 *
 *  Created on: Nov 20, 2024
 *      Author: alex9932
 */

#ifndef _RENDERCTX_H
#define _RENDERCTX_H

#include "rendertypes.h"

// TODO: Make new render backend API

/*

Structs:
 RenderDevice*  // Render backend state
 Pipeline*      // Rendering pipeline
 CommandQueue*  // GPU command buffer
 MemoryBuffer*  // Data buffer
 ResouceView*   // Buffer access in shaders

 RenderSetupInfo // Render backend setup parameters
 CommandQueueSubmitInfo // Command queue submission parameters

 *CreateInfo     // Resource creation parameters

Backend proc (marked as backend functions):
 R_CreateDevice(RenderSetupInfo*) -> RenderDevice*
 R_DestroyDevice(RenderDevice*)
 R_CreatePipeline(RenderDevice*, PipelineCreateInfo*) -> Pipeline*
 R_DestroyPipeline(Pipeline*)
 R_CreateCommandQueue(RenderDevice*, CommandQueueCreateInfo*) -> CommandQueue*
 R_DestroyCommandQueue(CommandQueue*)
 R_CreateMemoryBuffer(RenderDevice*, MemoryBufferCreateInfo*) -> MemoryBuffer*
 R_DestroyMemoryBuffer(MemoryBuffer*)
 R_CreateResourceView(RenderDevice*, ResourceViewCreateInfo*) -> ResourceView*
 R_DestroyResourceView(ResourceView*)
 R_SubmitCommandQueue(CommandQueueSubmitInfo*)
 R_WaitIdle(RenderDevice*)    ?
 R_SwapBuffers(RenderDevice*) ?

 R_CMD_Begin(CommandQueue*)
 R_CMD_End(CommandQueue*)
 R_CMD_SetPipeline(CommandQueue*, Pipeline*)
 R_CMD_Draw(CommandQueue*, Uint32 vertexCount, Uint32 instanceCount, Uint32 firstVertex, Uint32 firstInstance)
 R_CMD_Dispatch(CommandQueue*, Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ)
 R_CMD_CopyBuffer(CommandQueue*, MemoryBuffer* src, MemoryBuffer* dst, Uint64 size, Uint64 srcOffset, Uint64 dstOffset)
?R_CMD_UpdateBuffer(CommandQueue*, MemoryBuffer* dst, const void* data, Uint64 size, Uint64 dstOffset)
 R_CMD_ClearRenderTarget(CommandQueue*, RenderTarget*, Float32 r, Float32 g, Float32 b, Float32 a)
 R_CMD_ClearDepthStencil(CommandQueue*, DepthStencil*, Float32 depth, Uint8 stencil)
 R_CMD_BeginRenderPass(CommandQueue*, RenderPassBeginInfo*)
 R_CMD_EndRenderPass(CommandQueue*)
?R_CMD_SetViewport(CommandQueue*, Uint32 x, Uint32 y, Uint32 width, Uint32 height)
 R_CMD_PushConstants(CommandQueue*, const void* data, Uint32 size, Uint32 offset)
 R_CMD_BindRenderTargets(CommandQueue*, RenderTarget**, Uint32 rt_count, DepthStencil*)
?R_CMD_BindComputeResources(CommandQueue*, ResourceView**, Uint32 res_count, Uint32 start_slot)
?R_CMD_BindGraphicsResources(CommandQueue*, ResourceView**, Uint32 res_count, Uint32 start_slot)

*/



// Render core
typedef SDL_Window*  (*PFN_R_SHOWWINDOW)(Uint32, Uint32); // Width, height
typedef void         (*PFN_R_SETUP)(RenderSetupInfo*);
typedef void         (*PFN_R_INITIALIZE)(SDL_Window*);
typedef void         (*PFN_R_DESTROY)();
typedef void         (*PFN_R_SWAPBUFFERS)();
typedef void         (*PFN_R_GETINFO)(RenderInfo*);


// TODO: Make it deprecated

// R2D
typedef R2D_Buffer*  (*PFN_R2D_CREATEBUFFER)(R2DCreateBufferInfo*);
typedef void         (*PFN_R2D_DESTROYBUFFER)(R2D_Buffer*);
typedef void         (*PFN_R2D_BUFFERDATA)(R2DBufferDataInfo*);

typedef R2D_Texture* (*PFN_R2D_CREATETEXTURE)(R2DCreateTextureInfo*);
typedef R2D_Texture* (*PFN_R2D_CREATEMEMTEXTURE)(R2DCreateMemTextureInfo*);
typedef void         (*PFN_R2D_DESTROYTEXTURE)(R2D_Texture*);
typedef void         (*PFN_R2D_TEXTUREDATA)(R2DTextureDataInfo*);

typedef void         (*PFN_R2D_PUSHMATRIX)(mat4*);
typedef mat4*        (*PFN_R2D_POPMATRIX)();
typedef void         (*PFN_R2D_RESETSTACK)();

typedef void         (*PFN_R2D_BEGIN)();
typedef void         (*PFN_R2D_BIND)(R2DBindInfo*);
typedef void         (*PFN_R2D_DRAW)(R2DDrawInfo*);

// R3D
typedef R3D_Material*       (*PFN_R3D_CREATEMATERIAL)(R3DCreateMaterialInfo*);
typedef void                (*PFN_R3D_DESTROYMATERIAL)(R3D_Material*);

typedef R3D_StaticModel*    (*PFN_R3D_CREATESTATICMODEL)(R3DStaticModelInfo*);
typedef void                (*PFN_R3D_DESTROYSTATICMODEL)(R3D_StaticModel*);

typedef R3D_RiggedModel*    (*PFN_R3D_CREATERIGGEDMODEL)(R3DRiggedModelInfo*);
typedef void                (*PFN_R3D_DESTROYRIGGEDMODEL)(R3D_RiggedModel*);

typedef R3D_BoneBuffer*     (*PFN_R3D_CREATEBONEBUFFER)(R3DCreateBufferInfo*);
typedef void                (*PFN_R3D_DESTROYBONEBUFFER)(R3D_BoneBuffer*);
typedef void                (*PFN_R3D_UPDATEBONEBUFFER)(R3DUpdateBufferInfo*);

typedef R3D_AtlasHandle*    (*PFN_R3D_CREATEATLAS)(String);
typedef void                (*PFN_R3D_DESTROYATLAS)(R3D_AtlasHandle*);

typedef R3D_ParticleBuffer* (*PFN_R3D_CREATEPARTICLEBUFFER)(R3DCreateBufferInfo*);
typedef void                (*PFN_R3D_DESTROYPARTICLEBUFFER)(R3D_ParticleBuffer*);
typedef void                (*PFN_R3D_UPDATEPARTICLEBUFFER)(R3DUpdateBufferInfo*);

typedef void                (*PFN_R3D_PUSHLIGHTSOURCE)(R3D_LightSource*);
typedef void                (*PFN_R3D_PUSHMODEL)(R3D_PushModelInfo*);
typedef void                (*PFN_R3D_SETCAMERA)(R3D_CameraInfo*);

typedef void			    (*PFN_R3D_STARTRENDERTASK)(R3D_RenderTaskInfo*);


namespace Engine {

	typedef struct Renderer {

		// Core
		PFN_R_SHOWWINDOW              ShowWindow;
		PFN_R_SETUP                   Setup;
		PFN_R_INITIALIZE              Initialize;
		PFN_R_DESTROY                 Destroy;
		PFN_R_SWAPBUFFERS             SwapBuffers;
		PFN_R_GETINFO                 GetInfo;

		// R2D
		PFN_R2D_CREATEBUFFER          R2D_CreateBuffer;
		PFN_R2D_DESTROYBUFFER         R2D_DestroyBuffer;
		PFN_R2D_BUFFERDATA            R2D_BufferData;
		PFN_R2D_CREATETEXTURE         R2D_CreateTexture;
		PFN_R2D_CREATEMEMTEXTURE      R2D_CreateMemTexture;
		PFN_R2D_DESTROYTEXTURE        R2D_DestroyTexture;
		PFN_R2D_TEXTUREDATA           R2D_TextureData;
		PFN_R2D_PUSHMATRIX            R2D_PushMatrix;
		PFN_R2D_POPMATRIX             R2D_PopMatrix;
		PFN_R2D_RESETSTACK            R2D_ResetStack;
		PFN_R2D_BEGIN                 R2D_Begin;
		PFN_R2D_BIND                  R2D_Bind;
		PFN_R2D_DRAW                  R2D_Draw;

		// R3D
		PFN_R3D_CREATEMATERIAL        R3D_CreateMaterial;
		PFN_R3D_DESTROYMATERIAL       R3D_DestroyMaterial;

		PFN_R3D_CREATESTATICMODEL     R3D_CreateStaticModel;
		PFN_R3D_DESTROYSTATICMODEL    R3D_DestroyStaticModel;

		PFN_R3D_CREATERIGGEDMODEL     R3D_CreateRiggedModel;
		PFN_R3D_DESTROYRIGGEDMODEL    R3D_DestroyRiggedModel;

		PFN_R3D_CREATEBONEBUFFER      R3D_CreateBoneBuffer;
		PFN_R3D_DESTROYBONEBUFFER     R3D_DestroyBoneBuffer;
		PFN_R3D_UPDATEBONEBUFFER      R3D_UpdateBoneBuffer;

		PFN_R3D_CREATEATLAS           R3D_CreateAtlas;
		PFN_R3D_DESTROYATLAS          R3D_DestroyAtlas;

		PFN_R3D_CREATEPARTICLEBUFFER  R3D_CreateParticleBuffer;
		PFN_R3D_DESTROYPARTICLEBUFFER R3D_DestroyParticleBuffer;
		PFN_R3D_UPDATEPARTICLEBUFFER  R3D_UpdateParticleBuffer;

		PFN_R3D_PUSHLIGHTSOURCE       R3D_PushLightSource;
		PFN_R3D_PUSHMODEL             R3D_PushModel;
		PFN_R3D_SETCAMERA             R3D_SetCamera;

		PFN_R3D_STARTRENDERTASK       R3D_StartRenderTask;

	} Renderer;

	void LoadRendererContext(Renderer* ctx, LibraryHandle handle);
	void ClearRendererContext(Renderer* ctx);

}

#endif
