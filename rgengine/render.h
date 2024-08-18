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

// R2D
typedef R2D_Buffer*  (*PFN_R2D_CREATEBUFFER)(R2DCreateBufferInfo*);
typedef void         (*PFN_R2D_DESTROYBUFFER)(R2D_Buffer*);
typedef void         (*PFN_R2D_BUFFERDATA)(R2DBufferDataInfo*);

typedef R2D_Texture* (*PFN_R2D_CREATETEXTURE)(R2DCreateTextureInfo*);
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

typedef void                (*PFN_R3D_PUSHMODEL)(R3D_PushModelInfo*);
typedef void                (*PFN_R3D_SETCAMERA)(R3D_CameraInfo*);

typedef void			    (*PFN_R3D_STARTRENDERTASK)(R3D_RenderTaskInfo*);

namespace Engine {

	class ModelSystem;
	class LightSystem;
	class ParticleSystem;

	namespace Render {

		// Core
		extern RG_DECLSPEC PFN_R_SHOWWINDOW              ShowWindow;
		extern RG_DECLSPEC PFN_R_SETUP                   Setup;
		extern RG_DECLSPEC PFN_R_INITIALIZE              Initialize;
		extern RG_DECLSPEC PFN_R_DESTROY                 Destroy;
		extern RG_DECLSPEC PFN_R_SWAPBUFFERS             SwapBuffers;
		extern RG_DECLSPEC PFN_R_GETINFO                 GetInfo;

		// R2D
		extern RG_DECLSPEC PFN_R2D_CREATEBUFFER          R2D_CreateBuffer;
		extern RG_DECLSPEC PFN_R2D_DESTROYBUFFER         R2D_DestroyBuffer;
		extern RG_DECLSPEC PFN_R2D_BUFFERDATA            R2D_BufferData;
		extern RG_DECLSPEC PFN_R2D_CREATETEXTURE         R2D_CreateTexture;
		extern RG_DECLSPEC PFN_R2D_DESTROYTEXTURE        R2D_DestroyTexture;
		extern RG_DECLSPEC PFN_R2D_TEXTUREDATA           R2D_TextureData;
		extern RG_DECLSPEC PFN_R2D_PUSHMATRIX            R2D_PushMatrix;
		extern RG_DECLSPEC PFN_R2D_POPMATRIX             R2D_PopMatrix;
		extern RG_DECLSPEC PFN_R2D_RESETSTACK            R2D_ResetStack;
		extern RG_DECLSPEC PFN_R2D_BEGIN                 R2D_Begin;
		extern RG_DECLSPEC PFN_R2D_BIND                  R2D_Bind;
		extern RG_DECLSPEC PFN_R2D_DRAW                  R2D_Draw;

		// R3D
		extern RG_DECLSPEC PFN_R3D_CREATEMATERIAL        R3D_CreateMaterial;
		extern RG_DECLSPEC PFN_R3D_DESTROYMATERIAL       R3D_DestroyMaterial;

		extern RG_DECLSPEC PFN_R3D_CREATESTATICMODEL     R3D_CreateStaticModel;
		extern RG_DECLSPEC PFN_R3D_DESTROYSTATICMODEL    R3D_DestroyStaticModel;

		extern RG_DECLSPEC PFN_R3D_CREATERIGGEDMODEL     R3D_CreateRiggedModel;
		extern RG_DECLSPEC PFN_R3D_DESTROYRIGGEDMODEL    R3D_DestroyRiggedModel;

		extern RG_DECLSPEC PFN_R3D_CREATEBONEBUFFER      R3D_CreateBoneBuffer;
		extern RG_DECLSPEC PFN_R3D_DESTROYBONEBUFFER     R3D_DestroyBoneBuffer;
		extern RG_DECLSPEC PFN_R3D_UPDATEBONEBUFFER      R3D_UpdateBoneBuffer;

		extern RG_DECLSPEC PFN_R3D_CREATEATLAS           R3D_CreateAtlas;
		extern RG_DECLSPEC PFN_R3D_DESTROYATLAS          R3D_DestroyAtlas;

		extern RG_DECLSPEC PFN_R3D_CREATEPARTICLEBUFFER  R3D_CreateParticleBuffer;
		extern RG_DECLSPEC PFN_R3D_DESTROYPARTICLEBUFFER R3D_DestroyParticleBuffer;
		extern RG_DECLSPEC PFN_R3D_UPDATEPARTICLEBUFFER  R3D_UpdateParticleBuffer;

		extern RG_DECLSPEC PFN_R3D_PUSHMODEL             R3D_PushModel;
		extern RG_DECLSPEC PFN_R3D_SETCAMERA             R3D_SetCamera;

		RG_DECLSPEC void          LoadRenderer(String path);
		RG_DECLSPEC void          UnloadRenderer();
		RG_DECLSPEC Bool          IsRendererLoaded();
		RG_DECLSPEC LibraryHandle GetHandle();

		void InitSubSystem();
		void DestroySubSystem();

		// ImGui window
		RG_DECLSPEC void DrawRendererStats();
		RG_DECLSPEC void DrawProfilerStats();

		RG_DECLSPEC void SetCamera(R3D_CameraInfo* info);

		RG_DECLSPEC void UpdateSystems();
		RG_DECLSPEC void Update();

		RG_DECLSPEC void SetGlobalLight(R3D_GlobalLightDescrition* desc);

		//RG_DECLSPEC void ToggleConsole();

		RG_DECLSPEC ModelSystem* GetModelSystem();
		RG_DECLSPEC LightSystem* GetLightSystem();
		RG_DECLSPEC ParticleSystem* GetParticleSystem();

		RG_DECLSPEC R3D_BoneBuffer* CreateBoneBuffer(R3DCreateBufferInfo* info);
		RG_DECLSPEC void DestroyBoneBuffer(R3D_BoneBuffer* hbuff);
		RG_DECLSPEC void UpdateBoneBuffer(R3DUpdateBufferInfo* info);

		RG_DECLSPEC R3D_AtlasHandle* CreateAtlas(String texture);
		RG_DECLSPEC void DestroyAtlas(R3D_AtlasHandle* atlas);

		RG_DECLSPEC R3D_ParticleBuffer* CreateParticleBuffer(R3DCreateBufferInfo* info);
		RG_DECLSPEC void DestroyParticleBuffer(R3D_ParticleBuffer* hbuff);
		RG_DECLSPEC void UpdateParticleBuffer(R3DUpdateBufferInfo* info);

		RG_DECLSPEC RenderSetupInfo* GetSetupParams();
		RG_DECLSPEC void SetRenderFlags(Uint32 flags);

	}
}

#endif