/*
 * rgEngine renderctx.h
 *
 *  Created on: Nov 20, 2024
 *      Author: alex9932
 */

#ifndef _RENDERCTX_H
#define _RENDERCTX_H

#include "rendertypes.h"


 // Render core
typedef SDL_Window* (*PFN_R_SHOWWINDOW)(Uint32, Uint32); // Width, height
typedef void              (*PFN_R_SETUP)(RenderSetupInfo*);
typedef void              (*PFN_R_INITIALIZE)(SDL_Window*);
typedef void              (*PFN_R_DESTROY)();
typedef void              (*PFN_R_SWAPBUFFERS)();
typedef void              (*PFN_R_GETINFO)(RenderInfo*);

// R2D
typedef R2D_Buffer* (*PFN_R2D_CREATEBUFFER)(R2DCreateBufferInfo*);
typedef void         (*PFN_R2D_DESTROYBUFFER)(R2D_Buffer*);
typedef void         (*PFN_R2D_BUFFERDATA)(R2DBufferDataInfo*);

typedef R2D_Texture* (*PFN_R2D_CREATETEXTURE)(R2DCreateTextureInfo*);
typedef void         (*PFN_R2D_DESTROYTEXTURE)(R2D_Texture*);
typedef void         (*PFN_R2D_TEXTUREDATA)(R2DTextureDataInfo*);

typedef void         (*PFN_R2D_PUSHMATRIX)(mat4*);
typedef mat4* (*PFN_R2D_POPMATRIX)();
typedef void         (*PFN_R2D_RESETSTACK)();

typedef void         (*PFN_R2D_BEGIN)();
typedef void         (*PFN_R2D_BIND)(R2DBindInfo*);
typedef void         (*PFN_R2D_DRAW)(R2DDrawInfo*);

// R3D
typedef R3D_Material* (*PFN_R3D_CREATEMATERIAL)(R3DCreateMaterialInfo*);
typedef void                (*PFN_R3D_DESTROYMATERIAL)(R3D_Material*);

typedef R3D_StaticModel* (*PFN_R3D_CREATESTATICMODEL)(R3DStaticModelInfo*);
typedef void                (*PFN_R3D_DESTROYSTATICMODEL)(R3D_StaticModel*);

typedef R3D_RiggedModel* (*PFN_R3D_CREATERIGGEDMODEL)(R3DRiggedModelInfo*);
typedef void                (*PFN_R3D_DESTROYRIGGEDMODEL)(R3D_RiggedModel*);

typedef R3D_BoneBuffer* (*PFN_R3D_CREATEBONEBUFFER)(R3DCreateBufferInfo*);
typedef void                (*PFN_R3D_DESTROYBONEBUFFER)(R3D_BoneBuffer*);
typedef void                (*PFN_R3D_UPDATEBONEBUFFER)(R3DUpdateBufferInfo*);

typedef R3D_AtlasHandle* (*PFN_R3D_CREATEATLAS)(String);
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
