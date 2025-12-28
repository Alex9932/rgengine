#ifndef _RENDER_H
#define _RENDER_H

#include "rgtypes.h"
#include "rendertypes.h"

#include "renderctx.h"

#define RG_EVENT_RENDER_VIEWPORT_RESIZE 0x00010001


enum ModelType {
	R_MODEL_STATIC = 0,
	R_MODEL_RIGGED = 1
};

typedef struct R3D_StaticModel {
    ModelType     type;
    Uint32        mCount;
    R3D_MeshInfo* info;
    RBuffer*      vBuffer;
    RBuffer*      iBuffer;
    Uint32        iCount;
    IndexType     iType;
} R3D_StaticModel;

typedef struct R3D_RiggedModel {
    ModelType       type;
    Uint32          vCount;
    // Input data
    RBuffer*        i_vertex;    // Input vertex data
    RBuffer*        i_weight;    // Input weight data
    //RResourceView*  i_srv_vtx; // Shader resource view for vertex input data
    //RResourceView*  i_srv_wht; // Shader resource view for weight input data
    // Output data
    R3D_StaticModel s_model;     // Static model / output vertex data
    //RResourceView*  s_uav;     // Unordered access view for vertex output data

	RDescriptorSet* set;         // Descriptor set binds all buffers simultaneously
} R3D_RiggedModel;

typedef struct R3D_BoneBuffer {
    RBuffer*       buffer;
	RDescriptorSet* set;
    //RResourceView* rv;
} R3D_BoneBuffer;

namespace Engine {

	class ModelSystem;
	class LightSystem;
	class ParticleSystem;

	namespace Render {

		RG_DECLSPEC void          LoadRenderer(String path);
		RG_DECLSPEC void          UnloadRenderer();
		RG_DECLSPEC Bool          IsRendererLoaded();
		RG_DECLSPEC LibraryHandle GetHandle();

		void InitSubSystem(SDL_Window* hwnd);
		void DestroySubSystem();


		SDL_Window* ShowWindow(Uint32 w, Uint32 h);
		//void InitializeContext();
		void SwapBuffers();

		RRenderDevice* GetRenderDevice();
		RenderBackend* GetRenderContext();

		RG_DECLSPEC void SetCamera(R3D_CameraInfo* info);

		RG_DECLSPEC void UpdateSystems();
		RG_DECLSPEC void Update();

		RG_DECLSPEC void SetGlobalLight(R3D_GlobalLightDescrition* desc);
		RG_DECLSPEC void GetInfo(RenderInfo* info);

		RG_DECLSPEC ParticleSystem* GetParticleSystem();


		RG_DECLSPEC R3D_StaticModel* CreateStaticModel(R3DStaticModelInfo* info);
		RG_DECLSPEC void DestroyStaticModel(R3D_StaticModel* mdl);

		RG_DECLSPEC R3D_RiggedModel* CreateRiggedModel(R3DRiggedModelInfo* info);
		RG_DECLSPEC void DestroyRiggedModel(R3D_RiggedModel* mdl);

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