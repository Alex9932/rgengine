/*
 * rgEngine renderctx.h
 *
 *  Created on: Nov 20, 2024
 *      Author: alex9932
 */

#ifndef _RENDERCTX_H
#define _RENDERCTX_H

#include "rendertypes.h"

typedef SDL_Window*     (*PFN_R_SHOWWINDOW)(Uint32, Uint32); // Width, height
typedef void            (*PFN_R_SETUP)();

typedef RRenderDevice*  (*PFN_R_CREATEDEVICE)(RRenderSetupInfo*);                  // PFN_R_INITIALIZE
typedef void            (*PFN_R_DESTROYDEVICE)(RRenderDevice*);                    // PFN_R_DESTROY
typedef void            (*PFN_R_GETINFO)(RRenderDevice*, RenderInfo*);
typedef void            (*PFN_R_SWAPBUFFERS)(RRenderDevice*, RSwapBuffersInfo*);
typedef void            (*PFN_R_WAITIDLE)(RRenderDevice*);

typedef void            (*PFN_R_IMGUI_INIT)(RRenderDevice*);
typedef void            (*PFN_R_IMGUI_SHUTDOWN)(RRenderDevice*);
typedef void            (*PFN_R_IMGUI_NEWFRAME)(RRenderDevice*);
typedef void*           (*PFN_R_IMGUI_ADDTEXTURE)(RRenderDevice*, RImage*);
typedef void            (*PFN_R_IMGUI_REMOVETEXTURE)(void*);

typedef RBuffer*        (*PFN_R_CREATEBUFFER)(RRenderDevice*, RBufferCreateInfo*);
typedef void            (*PFN_R_DESTROYBUFFER)(RBuffer*);
typedef void            (*PFN_R_UPDATEBUFFER)(RUpdateBufferInfo*);
typedef RImage*         (*PFN_R_CREATEIMAGE)(RRenderDevice*, RImageCreateInfo*);
typedef void            (*PFN_R_DESTROYIMAGE)(RImage*);
typedef RFramebuffer*   (*PFN_R_CREATEFRAMEBUFFER)(RRenderDevice*, RFramebufferCreateInfo*);
typedef void            (*PFN_R_DESTROYFRAMEBUFFER)(RFramebuffer*);
typedef RDescriptorSet* (*PFN_R_CREATEDESCRIPTORSET)(RRenderDevice*, RDescriptorSetCreateInfo*);
typedef void            (*PFN_R_DESTROYDESCRIPTORSET)(RDescriptorSet*);

typedef RRenderpass*    (*PFN_R_CREATERENDERPASS)(RRenderDevice*, RRenderpassCreateInfo*);
typedef void            (*PFN_R_DESTROYRENDERPASS)(RRenderpass*);
typedef RPipeline*      (*PFN_R_CREATEPIPELINE)(RRenderDevice*, RPipelineCreateInfo*);
typedef void            (*PFN_R_DESTROYPIPELINE)(RPipeline*);
typedef RShader*        (*PFN_R_CREATESHADER)(RRenderDevice*, RShaderCreateInfo*);
typedef void            (*PFN_R_DESTROYSHADER)(RShader*);
typedef RSampler*       (*PFN_R_CREATESAMPLER)(RRenderDevice*, RSamplerCreateInfo*);
typedef void            (*PFN_R_DESTROYSAMPLER)(RSampler*);

typedef RCommandBuffer* (*PFN_R_CREATECOMMANDBUFFER)(RRenderDevice*, RCommandBufferCreateInfo*);
typedef void            (*PFN_R_DESTROYCOMMANDBUFFER)(RCommandBuffer*);
typedef void            (*PFN_R_SUBMITCOMMANDBUFFER)(RCommandBufferSubmitInfo*);
typedef void            (*PFN_R_RESETCOMMANDBUFFER)(RCommandBuffer*);
typedef void            (*PFN_R_BEGINCOMMANDBUFFER)(RCommandBuffer*);
typedef void            (*PFN_R_ENDCOMMANDBUFFER)(RCommandBuffer*);

typedef void            (*PFN_R_CMDBEGINRENDERPASS)(RCommandBuffer*, RRenderpassBeginInfo*);
typedef void            (*PFN_R_CMDENDRENDERPASS)(RCommandBuffer*);
typedef void            (*PFN_R_CMDBINDPIPELINE)(RCommandBuffer*, RPipeline*);
typedef void            (*PFN_R_CMDBINDVERTEXBUFFER)(RCommandBuffer*, RBuffer*, Uint32, Uint32);
typedef void            (*PFN_R_CMDBINDINDEXBUFFER)(RCommandBuffer*, RBuffer*, IndexType);
typedef void            (*PFN_R_CMDBINDDESCRIPTORSETS)(RCommandBuffer*, RBindDescriptorSetsInfo*);
typedef void            (*PFN_R_CMDBINDSAMPLER)(RCommandBuffer*, RSampler*, Uint32, Uint32);
typedef void            (*PFN_R_CMDDRAWINDEXED)(RCommandBuffer*, Uint32, Uint32);
typedef void            (*PFN_R_CMDPUSHCONSTANTS)(RCommandBuffer*, void*, Uint32, Uint32);
typedef void            (*PFN_R_CMDDISPATCH)(RCommandBuffer*, Uint32, Uint32, Uint32);
typedef void            (*PFN_R_CMDUSEIMAGE)(RCommandBuffer*, RImage*, Uint32);

typedef void            (*PFN_R_CMDIMGUIRENDERDRAWDATA)(RCommandBuffer*, void*);

namespace Engine {
	typedef struct RenderBackend {

		// Core
		PFN_R_SHOWWINDOW              ShowWindow;
		PFN_R_SETUP                   Setup;
		PFN_R_CREATEDEVICE            CreateDevice;
		PFN_R_DESTROYDEVICE           DestroyDevice;
		PFN_R_SWAPBUFFERS             SwapBuffers;
		PFN_R_GETINFO                 GetInfo;
		PFN_R_WAITIDLE                WaitIdle;

		// ImGUI
		PFN_R_IMGUI_INIT              ImGui_Init;
		PFN_R_IMGUI_SHUTDOWN          ImGui_Shutdown;
		PFN_R_IMGUI_NEWFRAME          ImGui_NewFrame;
		PFN_R_IMGUI_ADDTEXTURE        ImGui_AddTexture;
		PFN_R_IMGUI_REMOVETEXTURE     ImGui_RemoveTexture;

		// Buffer
		PFN_R_CREATEBUFFER            CreateBuffer;
		PFN_R_DESTROYBUFFER           DestroyBuffer;
		PFN_R_UPDATEBUFFER            UpdateBuffer;

		// Image
		PFN_R_CREATEIMAGE			  CreateImage;
		PFN_R_DESTROYIMAGE            DestroyImage;

		// Framebuffer
		PFN_R_CREATEFRAMEBUFFER       CreateFramebuffer;
		PFN_R_DESTROYFRAMEBUFFER      DestroyFramebuffer;

		// Command buffer
		PFN_R_CREATECOMMANDBUFFER     CreateCommandBuffer;
		PFN_R_DESTROYCOMMANDBUFFER    DestroyCommandBuffer;
		PFN_R_SUBMITCOMMANDBUFFER     SubmitCommandBuffer;
		PFN_R_RESETCOMMANDBUFFER      ResetCommandBuffer;
		PFN_R_BEGINCOMMANDBUFFER      BeginCommandBuffer;
		PFN_R_ENDCOMMANDBUFFER        EndCommandBuffer;

		// Descriptor sets
		PFN_R_CREATEDESCRIPTORSET     CreateDescriptorSet;
		PFN_R_DESTROYDESCRIPTORSET    DestroyDescriptorSet;

		// Renderpass
		PFN_R_CREATERENDERPASS		  CreateRenderpass;
		PFN_R_DESTROYRENDERPASS		  DestroyRenderpass;

		// Pipeline
		PFN_R_CREATEPIPELINE          CreatePipeline;
		PFN_R_DESTROYPIPELINE         DestroyPipeline;

		// Shader
		PFN_R_CREATESHADER            CreateShader;
		PFN_R_DESTROYSHADER           DestroyShader;

		// Sampler
		PFN_R_CREATESAMPLER           CreateSampler;
		PFN_R_DESTROYSAMPLER          DestroySampler;

		// Commands
		PFN_R_CMDBEGINRENDERPASS      CmdBeginRenderpass;
		PFN_R_CMDENDRENDERPASS        CmdEndRenderpass;
		PFN_R_CMDBINDPIPELINE         CmdBindPipeline;
		PFN_R_CMDBINDVERTEXBUFFER     CmdBindVertexBuffer;
		PFN_R_CMDBINDINDEXBUFFER      CmdBindIndexBuffer;
		PFN_R_CMDBINDSAMPLER          CmdBindSampler;
		PFN_R_CMDBINDDESCRIPTORSETS   CmdBindDescriptorSets;
		PFN_R_CMDDRAWINDEXED          CmdDrawIndexed;
		PFN_R_CMDPUSHCONSTANTS        CmdPushConstants;
		PFN_R_CMDDISPATCH             CmdDispatch;
		PFN_R_CMDIMGUIRENDERDRAWDATA  CmdImGuiRenderDrawData;
		PFN_R_CMDUSEIMAGE             CmdUseImage;

	} RenderBackend;

	void LoadRendererContext(RenderBackend* ctx, LibraryHandle handle);
	void ClearRendererContext(RenderBackend* ctx);

}

#endif
