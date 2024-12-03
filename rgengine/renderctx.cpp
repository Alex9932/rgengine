/*
 * rgEngine renderctx.cpp
 *
 *  Created on: Nov 20, 2024
 *      Author: alex9932
 */

#include "renderctx.h"
#include "engine.h"

namespace Engine {

	void LoadRendererContext(Renderer* ctx, LibraryHandle handle) {

        // Core
        ctx->ShowWindow  = (PFN_R_SHOWWINDOW)Engine::DL_GetProcAddress(handle, "R_ShowWindow");
        ctx->Setup       = (PFN_R_SETUP)Engine::DL_GetProcAddress(handle, "R_Setup");
        ctx->Initialize  = (PFN_R_INITIALIZE)Engine::DL_GetProcAddress(handle, "R_Initialize");
        ctx->Destroy     = (PFN_R_DESTROY)Engine::DL_GetProcAddress(handle, "R_Destroy");
        ctx->SwapBuffers = (PFN_R_SWAPBUFFERS)Engine::DL_GetProcAddress(handle, "R_SwapBuffers");
        ctx->GetInfo     = (PFN_R_GETINFO)Engine::DL_GetProcAddress(handle, "R_GetInfo");

        // R2D
        ctx->R2D_CreateBuffer   = (PFN_R2D_CREATEBUFFER)Engine::DL_GetProcAddress(handle, "R2D_CreateBuffer");
        ctx->R2D_DestroyBuffer  = (PFN_R2D_DESTROYBUFFER)Engine::DL_GetProcAddress(handle, "R2D_DestroyBuffer");
        ctx->R2D_BufferData     = (PFN_R2D_BUFFERDATA)Engine::DL_GetProcAddress(handle, "R2D_BufferData");
        ctx->R2D_CreateTexture  = (PFN_R2D_CREATETEXTURE)Engine::DL_GetProcAddress(handle, "R2D_CreateTexture");
        ctx->R2D_DestroyTexture = (PFN_R2D_DESTROYTEXTURE)Engine::DL_GetProcAddress(handle, "R2D_DestroyTexture");
        ctx->R2D_TextureData    = (PFN_R2D_TEXTUREDATA)Engine::DL_GetProcAddress(handle, "R2D_TextureData");
        ctx->R2D_PushMatrix     = (PFN_R2D_PUSHMATRIX)Engine::DL_GetProcAddress(handle, "R2D_PushMatrix");
        ctx->R2D_PopMatrix      = (PFN_R2D_POPMATRIX)Engine::DL_GetProcAddress(handle, "R2D_PopMatrix");
        ctx->R2D_ResetStack     = (PFN_R2D_RESETSTACK)Engine::DL_GetProcAddress(handle, "R2D_ResetStack");
        ctx->R2D_Begin          = (PFN_R2D_BEGIN)Engine::DL_GetProcAddress(handle, "R2D_Begin");
        ctx->R2D_Bind           = (PFN_R2D_BIND)Engine::DL_GetProcAddress(handle, "R2D_Bind");
        ctx->R2D_Draw           = (PFN_R2D_DRAW)Engine::DL_GetProcAddress(handle, "R2D_Draw");

        // R3D
        ctx->R3D_CreateMaterial        = (PFN_R3D_CREATEMATERIAL)Engine::DL_GetProcAddress(handle, "R3D_CreateMaterial");
        ctx->R3D_DestroyMaterial        = (PFN_R3D_DESTROYMATERIAL)Engine::DL_GetProcAddress(handle, "R3D_DestroyMaterial");

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

}