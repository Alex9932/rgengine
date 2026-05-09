/*
 * rgEngine renderctx.cpp
 *
 *  Created on: Nov 20, 2024
 *      Author: alex9932
 */

#include "renderctx.h"
#include "engine.h"

namespace Engine {

    void LoadRendererContext(RenderBackend* ctx, LibraryHandle handle) {
        // Core
        ctx->ShowWindow           = (PFN_R_SHOWWINDOW)Engine::DL_GetProcAddress(handle, "R_ShowWindow");
        ctx->Setup                = (PFN_R_SETUP)Engine::DL_GetProcAddress(handle, "R_Setup");
        ctx->CreateDevice         = (PFN_R_CREATEDEVICE)Engine::DL_GetProcAddress(handle, "R_CreateDevice");
        ctx->DestroyDevice        = (PFN_R_DESTROYDEVICE)Engine::DL_GetProcAddress(handle, "R_DestroyDevice");
        ctx->SwapBuffers          = (PFN_R_SWAPBUFFERS)Engine::DL_GetProcAddress(handle, "R_SwapBuffers");
        ctx->GetInfo              = (PFN_R_GETINFO)Engine::DL_GetProcAddress(handle, "R_GetInfo");
		ctx->WaitIdle             = (PFN_R_WAITIDLE)Engine::DL_GetProcAddress(handle, "R_WaitIdle");

		// ImGUI
		ctx->ImGui_Init           = (PFN_R_IMGUI_INIT)Engine::DL_GetProcAddress(handle, "R_ImGui_Init");
		ctx->ImGui_Shutdown       = (PFN_R_IMGUI_SHUTDOWN)Engine::DL_GetProcAddress(handle, "R_ImGui_Shutdown");
		ctx->ImGui_NewFrame       = (PFN_R_IMGUI_NEWFRAME)Engine::DL_GetProcAddress(handle, "R_ImGui_NewFrame");
		ctx->ImGui_AddTexture     = (PFN_R_IMGUI_ADDTEXTURE)Engine::DL_GetProcAddress(handle, "R_ImGui_AddTexture");
		ctx->ImGui_RemoveTexture  = (PFN_R_IMGUI_REMOVETEXTURE)Engine::DL_GetProcAddress(handle, "R_ImGui_RemoveTexture");

        // Buffer
		ctx->CreateBuffer         = (PFN_R_CREATEBUFFER)Engine::DL_GetProcAddress(handle, "R_CreateBuffer");
		ctx->DestroyBuffer        = (PFN_R_DESTROYBUFFER)Engine::DL_GetProcAddress(handle, "R_DestroyBuffer");
		ctx->UpdateBuffer         = (PFN_R_UPDATEBUFFER)Engine::DL_GetProcAddress(handle, "R_UpdateBuffer");

		// Image
		ctx->CreateImage          = (PFN_R_CREATEIMAGE)Engine::DL_GetProcAddress(handle, "R_CreateImage");
		ctx->DestroyImage         = (PFN_R_DESTROYIMAGE)Engine::DL_GetProcAddress(handle, "R_DestroyImage");

        // Framebuffer
        ctx->CreateFramebuffer    = (PFN_R_CREATEFRAMEBUFFER)Engine::DL_GetProcAddress(handle, "R_CreateFramebuffer");
        ctx->DestroyFramebuffer   = (PFN_R_DESTROYFRAMEBUFFER)Engine::DL_GetProcAddress(handle, "R_DestroyFramebuffer");

		// Command Queue
		ctx->CreateCommandBuffer  = (PFN_R_CREATECOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_CreateCommandBuffer");
		ctx->DestroyCommandBuffer = (PFN_R_DESTROYCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_DestroyCommandBuffer");
		ctx->SubmitCommandBuffer  = (PFN_R_SUBMITCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_SubmitCommandBuffer");
		ctx->ResetCommandBuffer   = (PFN_R_RESETCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_ResetCommandBuffer");
		ctx->BeginCommandBuffer   = (PFN_R_BEGINCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_BeginCommandBuffer");
		ctx->EndCommandBuffer     = (PFN_R_ENDCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_EndCommandBuffer");

        // Descriptor sets
        ctx->CreateDescriptorSet  = (PFN_R_CREATEDESCRIPTORSET)Engine::DL_GetProcAddress(handle, "R_CreateDescriptorSet");
        ctx->DestroyDescriptorSet = (PFN_R_DESTROYDESCRIPTORSET)Engine::DL_GetProcAddress(handle, "R_DestroyDescriptorSet");

        // Renderpass
		ctx->CreateRenderpass     = (PFN_R_CREATERENDERPASS)Engine::DL_GetProcAddress(handle, "R_CreateRenderpass");
		ctx->DestroyRenderpass    = (PFN_R_DESTROYRENDERPASS)Engine::DL_GetProcAddress(handle, "R_DestroyRenderpass");

        // Pipeline
		ctx->CreatePipeline       = (PFN_R_CREATEPIPELINE)Engine::DL_GetProcAddress(handle, "R_CreatePipeline");
		ctx->DestroyPipeline      = (PFN_R_DESTROYPIPELINE)Engine::DL_GetProcAddress(handle, "R_DestroyPipeline");

        // Shader
		ctx->CreateShader         = (PFN_R_CREATESHADER)Engine::DL_GetProcAddress(handle, "R_CreateShader");
		ctx->DestroyShader        = (PFN_R_DESTROYSHADER)Engine::DL_GetProcAddress(handle, "R_DestroyShader");

		ctx->CreateSampler        = (PFN_R_CREATESAMPLER)Engine::DL_GetProcAddress(handle, "R_CreateSampler");
		ctx->DestroySampler       = (PFN_R_DESTROYSAMPLER)Engine::DL_GetProcAddress(handle, "R_DestroySampler");

		// Commands
		ctx->CmdBeginRenderpass   = (PFN_R_CMDBEGINRENDERPASS)Engine::DL_GetProcAddress(handle, "R_CmdBeginRenderpass");
		ctx->CmdEndRenderpass     = (PFN_R_CMDENDRENDERPASS)Engine::DL_GetProcAddress(handle, "R_CmdEndRenderpass");
		ctx->CmdBindPipeline      = (PFN_R_CMDBINDPIPELINE)Engine::DL_GetProcAddress(handle, "R_CmdBindPipeline");
		ctx->CmdBindVertexBuffer  = (PFN_R_CMDBINDVERTEXBUFFER)Engine::DL_GetProcAddress(handle, "R_CmdBindVertexBuffer");
		ctx->CmdBindIndexBuffer   = (PFN_R_CMDBINDINDEXBUFFER)Engine::DL_GetProcAddress(handle, "R_CmdBindIndexBuffer");
        ctx->CmdBindDescriptorSets = (PFN_R_CMDBINDDESCRIPTORSETS)Engine::DL_GetProcAddress(handle, "R_CmdBindDescriptorSets");
        ctx->CmdBindSampler       = (PFN_R_CMDBINDSAMPLER)Engine::DL_GetProcAddress(handle, "R_CmdBindSampler");
		ctx->CmdDrawIndexed       = (PFN_R_CMDDRAWINDEXED)Engine::DL_GetProcAddress(handle, "R_CmdDrawIndexed");
        ctx->CmdPushConstants     = (PFN_R_CMDPUSHCONSTANTS)Engine::DL_GetProcAddress(handle, "R_CmdPushConstants");
		ctx->CmdDispatch          = (PFN_R_CMDDISPATCH)Engine::DL_GetProcAddress(handle, "R_CmdDispatch");
		ctx->CmdUseImage          = (PFN_R_CMDUSEIMAGE)Engine::DL_GetProcAddress(handle, "R_CmdUseImage");

        ctx->CmdImGuiRenderDrawData = (PFN_R_CMDIMGUIRENDERDRAWDATA)Engine::DL_GetProcAddress(handle, "R_CmdImGuiRenderDrawData");

    }

    void ClearRendererContext(RenderBackend* ctx) {
        SDL_memset(ctx, 0, sizeof(RenderBackend));
    }

}