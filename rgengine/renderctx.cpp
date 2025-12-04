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

		// ImGUI
		ctx->ImGui_Init           = (PFN_R_IMGUI_INIT)Engine::DL_GetProcAddress(handle, "R_ImGui_Init");
		ctx->ImGui_Shutdown       = (PFN_R_IMGUI_SHUTDOWN)Engine::DL_GetProcAddress(handle, "R_ImGui_Shutdown");
		ctx->ImGui_NewFrame       = (PFN_R_IMGUI_NEWFRAME)Engine::DL_GetProcAddress(handle, "R_ImGui_NewFrame");

        // Buffer
		ctx->CreateBuffer         = (PFN_R_CREATEBUFFER)Engine::DL_GetProcAddress(handle, "R_CreateBuffer");
		ctx->DestroyBuffer        = (PFN_R_DESTROYBUFFER)Engine::DL_GetProcAddress(handle, "R_DestroyBuffer");
		ctx->UpdateBuffer         = (PFN_R_UPDATEBUFFER)Engine::DL_GetProcAddress(handle, "R_UpdateBuffer");

		// Image
		ctx->CreateImage          = (PFN_R_CREATEIMAGE)Engine::DL_GetProcAddress(handle, "R_CreateImage");
		ctx->DestroyImage         = (PFN_R_DESTROYIMAGE)Engine::DL_GetProcAddress(handle, "R_DestroyImage");

		// Command Queue
		ctx->CreateCommandBuffer  = (PFN_R_CREATECOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_CreateCommandBuffer");
		ctx->DestroyCommandBuffer = (PFN_R_DESTROYCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_DestroyCommandBuffer");
		ctx->SubmitCommandBuffer  = (PFN_R_SUBMITCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_SubmitCommandBuffer");
		ctx->ResetCommandBuffer   = (PFN_R_RESETCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_ResetCommandBuffer");
		ctx->BeginCommandBuffer   = (PFN_R_BEGINCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_BeginCommandBuffer");
		ctx->EndCommandBuffer     = (PFN_R_ENDCOMMANDBUFFER)Engine::DL_GetProcAddress(handle, "R_EndCommandBuffer");

        // Resource view
		ctx->CreateResourceView   = (PFN_R_CREATERESOURCEVIEW)Engine::DL_GetProcAddress(handle, "R_CreateResourceView");
		ctx->DestroyResourceView  = (PFN_R_DESTROYRESOURCEVIEW)Engine::DL_GetProcAddress(handle, "R_DestroyResourceView");

        // Renderpass
		ctx->CreateRenderpass     = (PFN_R_CREATERENDERPASS)Engine::DL_GetProcAddress(handle, "R_CreateRenderpass");
		ctx->DestroyRenderpass    = (PFN_R_DESTROYRENDERPASS)Engine::DL_GetProcAddress(handle, "R_DestroyRenderpass");

        // Pipeline
		ctx->CreatePipeline       = (PFN_R_CREATEPIPELINE)Engine::DL_GetProcAddress(handle, "R_CreatePipeline");
		ctx->DestroyPipeline      = (PFN_R_DESTROYPIPELINE)Engine::DL_GetProcAddress(handle, "R_DestroyPipeline");

        // Shader
		ctx->CreateShader         = (PFN_R_CREATESHADER)Engine::DL_GetProcAddress(handle, "R_CreateShader");
		ctx->DestroyShader        = (PFN_R_DESTROYSHADER)Engine::DL_GetProcAddress(handle, "R_DestroyShader");

		// Commands
		ctx->CmdBeginRenderpass   = (PFN_R_CMDBEGINRENDERPASS)Engine::DL_GetProcAddress(handle, "R_CmdBeginRenderpass");
		ctx->CmdEndRenderpass     = (PFN_R_CMDENDRENDERPASS)Engine::DL_GetProcAddress(handle, "R_CmdEndRenderpass");
		ctx->CmdBindPipeline      = (PFN_R_CMDBINDPIPELINE)Engine::DL_GetProcAddress(handle, "R_CmdBindPipeline");
		ctx->CmdBindVertexBuffer  = (PFN_R_CMDBINDVERTEXBUFFER)Engine::DL_GetProcAddress(handle, "R_CmdBindVertexBuffer");
		ctx->CmdBindIndexBuffer   = (PFN_R_CMDBINDINDEXBUFFER)Engine::DL_GetProcAddress(handle, "R_CmdBindIndexBuffer");
		ctx->CmdBindResourceViews = (PFN_R_CMDBINDRESOURCEVIEWS)Engine::DL_GetProcAddress(handle, "R_CmdBindResourceViews");
		ctx->CmdDrawIndexed       = (PFN_R_CMDDRAWINDEXED)Engine::DL_GetProcAddress(handle, "R_CmdDrawIndexed");
        ctx->CmdPushConstants     = (PFN_R_CMDPUSHCONSTANTS)Engine::DL_GetProcAddress(handle, "R_CmdPushConstants");
		ctx->CmdDispatch          = (PFN_R_CMDDISPATCH)Engine::DL_GetProcAddress(handle, "R_CmdDispatch");

        ctx->CmdImGuiRenderDrawData = (PFN_R_CMDIMGUIRENDERDRAWDATA)Engine::DL_GetProcAddress(handle, "R_CmdImGuiRenderDrawData");

    }

    void ClearRendererContext(RenderBackend* ctx) {
        SDL_memset(ctx, 0, sizeof(RenderBackend));
    }

#if 0

	void LoadRendererContext(Renderer* ctx, LibraryHandle handle) {

        // Core
        ctx->ShowWindow  = (PFN_R_SHOWWINDOW)Engine::DL_GetProcAddress(handle, "R_ShowWindow");
        ctx->Setup       = (PFN_R_SETUP)Engine::DL_GetProcAddress(handle, "R_Setup");
        ctx->Initialize  = (PFN_R_INITIALIZE)Engine::DL_GetProcAddress(handle, "R_Initialize");
        ctx->Destroy     = (PFN_R_DESTROY)Engine::DL_GetProcAddress(handle, "R_Destroy");
        ctx->SwapBuffers = (PFN_R_SWAPBUFFERS)Engine::DL_GetProcAddress(handle, "R_SwapBuffers");
        ctx->GetInfo     = (PFN_R_GETINFO)Engine::DL_GetProcAddress(handle, "R_GetInfo");

        // R2D
        ctx->R2D_CreateBuffer     = (PFN_R2D_CREATEBUFFER)Engine::DL_GetProcAddress(handle, "R2D_CreateBuffer");
        ctx->R2D_DestroyBuffer    = (PFN_R2D_DESTROYBUFFER)Engine::DL_GetProcAddress(handle, "R2D_DestroyBuffer");
        ctx->R2D_BufferData       = (PFN_R2D_BUFFERDATA)Engine::DL_GetProcAddress(handle, "R2D_BufferData");
        ctx->R2D_CreateTexture    = (PFN_R2D_CREATETEXTURE)Engine::DL_GetProcAddress(handle, "R2D_CreateTexture");
        ctx->R2D_CreateMemTexture = (PFN_R2D_CREATEMEMTEXTURE)Engine::DL_GetProcAddress(handle, "R2D_CreateMemTexture");
        ctx->R2D_DestroyTexture   = (PFN_R2D_DESTROYTEXTURE)Engine::DL_GetProcAddress(handle, "R2D_DestroyTexture");
        ctx->R2D_TextureData      = (PFN_R2D_TEXTUREDATA)Engine::DL_GetProcAddress(handle, "R2D_TextureData");
        ctx->R2D_PushMatrix       = (PFN_R2D_PUSHMATRIX)Engine::DL_GetProcAddress(handle, "R2D_PushMatrix");
        ctx->R2D_PopMatrix        = (PFN_R2D_POPMATRIX)Engine::DL_GetProcAddress(handle, "R2D_PopMatrix");
        ctx->R2D_ResetStack       = (PFN_R2D_RESETSTACK)Engine::DL_GetProcAddress(handle, "R2D_ResetStack");
        ctx->R2D_Begin            = (PFN_R2D_BEGIN)Engine::DL_GetProcAddress(handle, "R2D_Begin");
        ctx->R2D_Bind             = (PFN_R2D_BIND)Engine::DL_GetProcAddress(handle, "R2D_Bind");
        ctx->R2D_Draw             = (PFN_R2D_DRAW)Engine::DL_GetProcAddress(handle, "R2D_Draw");

        // R3D
        ctx->R3D_CreateMaterial        = (PFN_R3D_CREATEMATERIAL)Engine::DL_GetProcAddress(handle, "R3D_CreateMaterial");
        ctx->R3D_DestroyMaterial       = (PFN_R3D_DESTROYMATERIAL)Engine::DL_GetProcAddress(handle, "R3D_DestroyMaterial");

        ctx->R3D_CreateStaticModel     = (PFN_R3D_CREATESTATICMODEL)Engine::DL_GetProcAddress(handle, "R3D_CreateStaticModel");
        ctx->R3D_DestroyStaticModel    = (PFN_R3D_DESTROYSTATICMODEL)Engine::DL_GetProcAddress(handle, "R3D_DestroyStaticModel");

        ctx->R3D_CreateRiggedModel     = (PFN_R3D_CREATERIGGEDMODEL)Engine::DL_GetProcAddress(handle, "R3D_CreateRiggedModel");
        ctx->R3D_DestroyRiggedModel    = (PFN_R3D_DESTROYRIGGEDMODEL)Engine::DL_GetProcAddress(handle, "R3D_DestroyRiggedModel");

        ctx->R3D_CreateBoneBuffer      = (PFN_R3D_CREATEBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_CreateBoneBuffer");
        ctx->R3D_DestroyBoneBuffer     = (PFN_R3D_DESTROYBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_DestroyBoneBuffer");
        ctx->R3D_UpdateBoneBuffer      = (PFN_R3D_UPDATEBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_UpdateBoneBuffer");

        ctx->R3D_CreateAtlas           = (PFN_R3D_CREATEATLAS)Engine::DL_GetProcAddress(handle, "R3D_CreateAtlas");
        ctx->R3D_DestroyAtlas          = (PFN_R3D_DESTROYATLAS)Engine::DL_GetProcAddress(handle, "R3D_DestroyAtlas");

        ctx->R3D_CreateParticleBuffer  = (PFN_R3D_CREATEPARTICLEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_CreateParticleBuffer");
        ctx->R3D_DestroyParticleBuffer = (PFN_R3D_DESTROYPARTICLEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_DestroyParticleBuffer");
        ctx->R3D_UpdateParticleBuffer  = (PFN_R3D_UPDATEPARTICLEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_UpdateParticleBuffer");

        ctx->R3D_PushLightSource       = (PFN_R3D_PUSHLIGHTSOURCE)Engine::DL_GetProcAddress(handle, "R3D_PushLightSource");
        ctx->R3D_PushModel             = (PFN_R3D_PUSHMODEL)Engine::DL_GetProcAddress(handle, "R3D_PushModel");
        ctx->R3D_SetCamera             = (PFN_R3D_SETCAMERA)Engine::DL_GetProcAddress(handle, "R3D_SetCamera");

        ctx->R3D_StartRenderTask       = (PFN_R3D_STARTRENDERTASK)Engine::DL_GetProcAddress(handle, "R3D_StartRenderTask");

	}

    void ClearRendererContext(Renderer* ctx) {
        SDL_memset(ctx, 0, sizeof(Renderer));
    }

#endif

}