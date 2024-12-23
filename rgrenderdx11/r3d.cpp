#define DLL_EXPORT
#include "r3d.h"
#include "dx11.h"
#include "rgrenderdx11.h"
#include <rshared.h>
#include <filesystem.h>

#include <rgmath.h>

#include "shader.h"
#include "gbuffer.h"
#include "shadowbuffer.h"
#include "lightpass.h"
#include "postprocess.h"

#include "particlepass.h"

#include "loader.h"

#include <profiler.h>

#define R_MATERIALS_COUNT 16000
#define R_MODELS_COUNT    16000
#define R_MAX_MODELS      16000
#define R_ATLASES_COUNT   4096

#define R_MAX_LIGHTS      1024

using namespace Engine;

static PoolAllocator*   alloc_materials;
static PoolAllocator*   alloc_staticmodels;
static PoolAllocator*   alloc_riggedmodels;
static LinearAllocator* alloc_matrices;
static LinearAllocator* alloc_lights;
static PoolAllocator*   alloc_bonebuffers;
static PoolAllocator*   alloc_atlases;
static PoolAllocator*   alloc_particlebuffers;

static RQueue* staticQueue;
static RQueue* riggedQueue;

static RQueue* lightQueue;

struct MatrixBuffer {
	mat4 viewproj;
	mat4 model;
};

struct ConstBuffer {
	vec4   color;
	vec3   camerapos;
	Uint32 __offset3;
};

static Shader* shader;
static Shader* skyshader;
static Shader* shadowshader;
static Buffer* mBuffer;
static Buffer* cBuffer;

static MatrixBuffer matrixBuffer;
static ConstBuffer  constBuffer;

static ComputeShader* skeletonShader;

// Camera
static mat4 cam_proj = {};
static mat4 cam_view = {};
static vec3 cam_pos  = {};
static vec3 cam_rot  = {};

// Stats
static Uint32 texturesLoaded = 0;
static Uint32 modelsLoaded   = 0;
static Uint32 drawCalls      = 0;
static Uint32 dispatchCalls  = 0;

// Skybox

static Float32 skybox_vertices[] = {
	// Position          Noraml   Tangent  UV
	-1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,

	-1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,

	 1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,

	-1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,

	-1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f,  1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,

	-1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f, -1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	-1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0,
	 1.0f, -1.0f,  1.0f, 0, 0, 0, 0, 0, 0, 0, 0
};

static Buffer* skyboxv;

static void LoadShaders() {

	// Compute shader
	char skel_cs[128];
	Engine::GetPath(skel_cs, 128, RG_PATH_SYSTEM, "shadersdx11/skeleton.cs");
	skeletonShader = RG_NEW_CLASS(RGetAllocator(), ComputeShader)(skel_cs, false);

	// Gbuffer shader
	InputDescription staticDescriptions[4] = {};
	staticDescriptions[0].name = "POSITION";
	staticDescriptions[0].inputSlot = 0;
	staticDescriptions[0].format = INPUT_R32G32B32_FLOAT;
	staticDescriptions[1].name = "NORMAL";
	staticDescriptions[1].inputSlot = 0;
	staticDescriptions[1].format = INPUT_R32G32B32_FLOAT;
	staticDescriptions[2].name = "TANGENT";
	staticDescriptions[2].inputSlot = 0;
	staticDescriptions[2].format = INPUT_R32G32B32_FLOAT;
	staticDescriptions[3].name = "VPOS";
	staticDescriptions[3].inputSlot = 0;
	staticDescriptions[3].format = INPUT_R32G32_FLOAT;
	PipelineDescription staticDescription = {};
	staticDescription.inputCount = 4;
	staticDescription.descriptions = staticDescriptions;

	char gbuff_vs[128];
	char gbuff_ps[128];
	Engine::GetPath(gbuff_vs, 128, RG_PATH_SYSTEM, "shadersdx11/gbuffer.vs");
	Engine::GetPath(gbuff_ps, 128, RG_PATH_SYSTEM, "shadersdx11/gbuffer_solid.ps");
	//Engine::GetPath(gbuff_ps, 128, RG_PATH_SYSTEM, "shadersdx11/gbuffer_transparent.ps");
	shader = RG_NEW_CLASS(RGetAllocator(), Shader)(&staticDescription, gbuff_vs, gbuff_ps, false);

	Engine::GetPath(gbuff_vs, 128, RG_PATH_SYSTEM, "shadersdx11/skybox.vs");
	Engine::GetPath(gbuff_ps, 128, RG_PATH_SYSTEM, "shadersdx11/skybox.ps");
	skyshader = RG_NEW_CLASS(RGetAllocator(), Shader)(&staticDescription, gbuff_vs, gbuff_ps, false);

	Engine::GetPath(gbuff_vs, 128, RG_PATH_SYSTEM, "shadersdx11/shadow.vs");
	Engine::GetPath(gbuff_ps, 128, RG_PATH_SYSTEM, "shadersdx11/shadow.ps");
	shadowshader = RG_NEW_CLASS(RGetAllocator(), Shader)(&staticDescription, gbuff_vs, gbuff_ps, false);

}

static void FreeShaders() {
	RG_DELETE_CLASS(RGetAllocator(), ComputeShader, skeletonShader);

	RG_DELETE_CLASS(RGetAllocator(), Shader, shader);
	RG_DELETE_CLASS(RGetAllocator(), Shader, skyshader);
	RG_DELETE_CLASS(RGetAllocator(), Shader, shadowshader);
}

void InitializeR3D(ivec2* size) {
	alloc_materials       = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_MaterialPool", R_MATERIALS_COUNT, sizeof(R3D_Material));
	alloc_staticmodels    = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_StaticModelPool", R_MODELS_COUNT, sizeof(R3D_StaticModel));
	alloc_riggedmodels    = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_RiggedModelPool", R_MODELS_COUNT, sizeof(R3D_RiggedModel));
	alloc_matrices        = RG_NEW_CLASS(RGetAllocator(), LinearAllocator)("R_MatrixPool", sizeof(mat4) * R_MODELS_COUNT * 2);
	alloc_lights          = RG_NEW_CLASS(RGetAllocator(), LinearAllocator)("R_LightPool",  sizeof(R3D_LightSource) * R_MAX_LIGHTS);
	alloc_bonebuffers     = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_BoneBuffersPool", R_MODELS_COUNT, sizeof(R3D_BoneBuffer));
	alloc_atlases         = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_AtlasHandlesPool", R_ATLASES_COUNT, sizeof(R3D_AtlasHandle));
	alloc_particlebuffers = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_ParticleBuffersPool", R_ATLASES_COUNT, sizeof(R3D_ParticleBuffer));
	//alloc_matrices     = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_MatrixPool", R_MODELS_COUNT * 2, sizeof(mat4));
	staticQueue = RG_NEW_CLASS(RGetAllocator(), RQueue)(R_MAX_MODELS * 2);
	riggedQueue = RG_NEW_CLASS(RGetAllocator(), RQueue)(R_MAX_MODELS * 3);

	lightQueue  = RG_NEW_CLASS(RGetAllocator(), RQueue)(R_MAX_LIGHTS);

	LoadShaders();

	// Skybox buffers
	BufferCreateInfo vbufferInfo = {};
	vbufferInfo.type = BUFFER_VERTEX;
	vbufferInfo.access = BUFFER_GPU_ONLY;
	vbufferInfo.usage = BUFFER_DEFAULT;
	vbufferInfo.length = sizeof(skybox_vertices);
	skyboxv = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);
	skyboxv->SetData(0, vbufferInfo.length, skybox_vertices);

	BufferCreateInfo bInfo = {};
	bInfo.access = BUFFER_CPU_WRITE;
	bInfo.usage = BUFFER_DYNAMIC;
	bInfo.type = BUFFER_CONSTANT;
	bInfo.length = sizeof(MatrixBuffer);
	mBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&bInfo);
	bInfo.length = sizeof(ConstBuffer);
	cBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&bInfo);

	ivec2 ssize = { 2048, 2048 };
	//ivec2 ssize = { 4096*2, 4096 * 2 };

	CreateShadowBuffer(&ssize);
	CreateGBuffer(size);
	CreateParticlePass(size);
	CreateLightpass(size);
	CreateFX(size);
}

void DestroyR3D() {

	DestroyShadowBuffer();
	DestroyLightpass();
	DestroyGBuffer();
	DestroyParticlePass();
	DestroyFX();

	FreeShaders();

	RG_DELETE_CLASS(RGetAllocator(), Buffer, mBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, cBuffer);

	RG_DELETE_CLASS(RGetAllocator(), Buffer, skyboxv);

	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_materials);
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_staticmodels);
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_riggedmodels);
	RG_DELETE_CLASS(RGetAllocator(), LinearAllocator, alloc_matrices);
	RG_DELETE_CLASS(RGetAllocator(), LinearAllocator, alloc_lights);
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_bonebuffers);
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_atlases);
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_particlebuffers);
	//RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_matrices);
	RG_DELETE_CLASS(RGetAllocator(), RQueue, staticQueue);
	RG_DELETE_CLASS(RGetAllocator(), RQueue, riggedQueue);
	RG_DELETE_CLASS(RGetAllocator(), RQueue, lightQueue);
	
}

void ResizeR3D(ivec2* wndSize) {

	//ResizeShadowBuffer(wndSize);
	ResizeGBuffer(wndSize);
	ResizeParticlePass(wndSize);
	ResizeLightpass(wndSize);
	ResizeFX(wndSize);

}

void ReloadShadersR3D() {
	FreeShaders();
	LoadShaders();
	ReloadShadersLightpass();
	ReloadShadersFX();
	ReloadParticleShaders();
}

RQueue* GetStaticQueue() { return staticQueue; }
RQueue* GetRiggedQueue() { return riggedQueue; }

mat4* GetCameraProjection() { return &cam_proj; }
mat4* GetCameraView()       { return &cam_view; }
vec3* GetCameraPosition()   { return &cam_pos; }
vec3* GetCameraRotation()   { return &cam_rot; }

//LinearAllocator* GetMatrixAllocator() { return alloc_matrices; }
//LinearAllocator* GetLightAllocator()  { return alloc_lights;   }

void GetR3DStats(R3DStats* stats) {
	stats->texturesLoaded = texturesLoaded;
	stats->modelsLoaded   = modelsLoaded;
	stats->drawCalls      = drawCalls;
	stats->dispatchCalls  = dispatchCalls;
}

////// PUBLIC API //////

// Material

R3D_Material* R3D_CreateMaterial(R3DCreateMaterialInfo* info) {
	R3D_Material* material = (R3D_Material*)alloc_materials->Allocate();

	//rgLogInfo(RG_LOG_RENDER, "Material: 0x%0.16lx", (RG_VPTR)material);
	rgLogInfo(RG_LOG_RENDER, "Material: %p", (RG_VPTR)material);

	LoaderPushTexture(info->albedo, &material->albedo);
	LoaderPushTexture(info->normal, &material->normal);
	LoaderPushTexture(info->pbr, &material->pbr);
	material->color = info->color;

	texturesLoaded += 3;

#if 0
	int w, h, c;
	Uint8* data = RG_STB_load_from_file(info->albedo, &w, &h, &c, 4);

	TextureInfo albedoInfo = {};
	albedoInfo.width    = w;
	albedoInfo.height   = h;
	albedoInfo.channels = c;
	albedoInfo.data     = data;

	data = RG_STB_load_from_file(info->normal, &w, &h, &c, 4);
	TextureInfo normalInfo = {};
	normalInfo.width    = w;
	normalInfo.height   = h;
	normalInfo.channels = c;
	normalInfo.data     = data;

	data = RG_STB_load_from_file(info->pbr, &w, &h, &c, 4);
	TextureInfo prbInfo = {};
	prbInfo.width    = w;
	prbInfo.height   = h;
	prbInfo.channels = c;
	prbInfo.data     = data;

	material->albedo = RG_NEW_CLASS(RGetAllocator(), Texture)(&albedoInfo);
	material->normal = RG_NEW_CLASS(RGetAllocator(), Texture)(&normalInfo);
	material->pbr    = RG_NEW_CLASS(RGetAllocator(), Texture)(&prbInfo);
	material->color  = info->color;

	RG_STB_image_free((Uint8*)albedoInfo.data);
	RG_STB_image_free((Uint8*)normalInfo.data);
	RG_STB_image_free((Uint8*)prbInfo.data);
#endif

	return material;
}

void R3D_DestroyMaterial(R3D_Material* hmat) {
	rgLogInfo(RG_LOG_RENDER, "Free material: %p", hmat);

	// TODO
	Texture* defaultTexture = GetDefaultTexture();

	if (hmat->albedo != defaultTexture) { TexturesDelete(hmat->albedo); texturesLoaded--; }
	if (hmat->normal != defaultTexture) { TexturesDelete(hmat->normal); texturesLoaded--; }
	if (hmat->pbr != defaultTexture)    { TexturesDelete(hmat->pbr);    texturesLoaded--; }

	//if (hmat->albedo != defaultTexture) { RG_DELETE_CLASS(RGetAllocator(), Texture, hmat->albedo); texturesLoaded--; }
	//if (hmat->normal != defaultTexture) { RG_DELETE_CLASS(RGetAllocator(), Texture, hmat->normal); texturesLoaded--; }
	//if (hmat->pbr != defaultTexture)    { RG_DELETE_CLASS(RGetAllocator(), Texture, hmat->pbr);    texturesLoaded--; }

	alloc_materials->Deallocate(hmat);
}

// Static model

R3D_StaticModel* R3D_CreateStaticModel(R3DStaticModelInfo* info) {
	R3D_StaticModel* staticModel = (R3D_StaticModel*)alloc_staticmodels->Allocate();
	staticModel->type = R_MODEL_STATIC;
	//info->modelType;

	rgLogInfo(RG_LOG_RENDER, "Static model: %p", staticModel);
	
	BufferCreateInfo vbufferInfo = {};
	vbufferInfo.type   = BUFFER_VERTEX;
	vbufferInfo.access = BUFFER_GPU_ONLY;
	vbufferInfo.usage  = BUFFER_DEFAULT;
	vbufferInfo.length = info->vCount * sizeof(R3D_Vertex);
	staticModel->vBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);
	staticModel->vBuffer->SetData(0, vbufferInfo.length, info->vertices);

	BufferCreateInfo ibufferInfo = {};
	ibufferInfo.type   = BUFFER_INDEX;
	ibufferInfo.access = BUFFER_GPU_ONLY;
	ibufferInfo.usage  = BUFFER_DEFAULT;
	ibufferInfo.length = info->iCount * info->iType;
	staticModel->iBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&ibufferInfo);
	staticModel->iBuffer->SetData(0, ibufferInfo.length, info->indices);

	staticModel->iCount = info->iCount;
	staticModel->iType  = info->iType;
	staticModel->mCount = info->mCount;

	staticModel->info = (R3D_MeshInfo*)RGetAllocator()->Allocate(sizeof(R3D_MeshInfo) * info->mCount);

	R3D_Material** materials = (R3D_Material**)RGetAllocator()->Allocate(sizeof(R3D_MeshInfo) * info->matCount);

	for (Uint32 i = 0; i < info->matCount; i++) {
		R3DCreateMaterialInfo creatematinfo = {};
		creatematinfo.albedo = info->matInfo[i].albedo;
		creatematinfo.normal = info->matInfo[i].normal;
		creatematinfo.pbr    = info->matInfo[i].pbr;
		creatematinfo.color  = info->matInfo[i].color;
		materials[i] = R3D_CreateMaterial(&creatematinfo);
	}

	for (Uint32 i = 0; i < info->mCount; i++) {
		staticModel->info[i].indexCount  = info->mInfo[i].indexCount;
		staticModel->info[i].indexOffset = info->mInfo[i].indexOffset;
		staticModel->info[i].material    = materials[info->mInfo[i].materialIdx];
	}

	RGetAllocator()->Deallocate(materials);

	modelsLoaded += staticModel->mCount;

	return staticModel;
}

static void FreeMaterials(R3D_StaticModel* model) {
	// TEMP
	std::vector<R3D_Material*> deleted;
	for (Uint32 i = 0; i < model->mCount; i++) {
		R3D_Material* mat = model->info[i].material;

		std::vector<R3D_Material*>::iterator it = deleted.begin();
		Bool del = true;
		for (; it != deleted.end(); it++) {
			if (*it == mat) {
				del = false;
			}
		}

		if (del) {
			deleted.push_back(mat);
			R3D_DestroyMaterial(mat);
		}
	}
}

void R3D_DestroyStaticModel(R3D_StaticModel* hsmdl) {
	rgLogInfo(RG_LOG_RENDER, "Free static model: %p", hsmdl);

	FreeMaterials(hsmdl);

	RG_DELETE_CLASS(RGetAllocator(), Buffer, hsmdl->vBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, hsmdl->iBuffer);
	RGetAllocator()->Deallocate(hsmdl->info);

	modelsLoaded += hsmdl->mCount;

	alloc_staticmodels->Deallocate(hsmdl);
}

// Rigged model

R3D_RiggedModel* R3D_CreateRiggedModel(R3DRiggedModelInfo* info) {
	R3D_RiggedModel* riggedModel = (R3D_RiggedModel*)alloc_riggedmodels->Allocate();
	riggedModel->type = R_MODEL_RIGGED;
	HRESULT result;

	rgLogInfo(RG_LOG_RENDER, "Rigged model: %p", riggedModel);

	// Static model
	BufferCreateInfo vbufferInfo = {};
	vbufferInfo.type      = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	//vbufferInfo.type      = BUFFER_UNORDERED;
	vbufferInfo.access    = BUFFER_GPU_ONLY;
	vbufferInfo.usage     = BUFFER_DEFAULT;
	vbufferInfo.length    = sizeof(R3D_Vertex) * info->vCount;
	vbufferInfo.miscflags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	vbufferInfo.stride    = sizeof(R3D_Vertex);
	riggedModel->s_model.vBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);
	//riggedModel->s_model.vBuffer->SetData(0, vbufferInfo.length, info->vertices);

	BufferCreateInfo ibufferInfo = {};
	ibufferInfo.type   = BUFFER_INDEX;
	ibufferInfo.access = BUFFER_GPU_ONLY;
	ibufferInfo.usage  = BUFFER_DEFAULT;
	ibufferInfo.length = info->iCount * info->iType;
	riggedModel->s_model.iBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&ibufferInfo);
	riggedModel->s_model.iBuffer->SetData(0, ibufferInfo.length, info->indices);

	riggedModel->s_model.iCount = info->iCount;
	riggedModel->s_model.iType  = info->iType;
	riggedModel->s_model.mCount = info->mCount;

	riggedModel->s_model.info = (R3D_MeshInfo*)RGetAllocator()->Allocate(sizeof(R3D_MeshInfo) * info->mCount);

	R3D_Material** materials = (R3D_Material**)RGetAllocator()->Allocate(sizeof(R3D_MeshInfo) * info->matCount);

	for (Uint32 i = 0; i < info->matCount; i++) {
		R3DCreateMaterialInfo creatematinfo = {};
		creatematinfo.albedo = info->matInfo[i].albedo;
		creatematinfo.normal = info->matInfo[i].normal;
		creatematinfo.pbr    = info->matInfo[i].pbr;
		creatematinfo.color  = info->matInfo[i].color;
		materials[i] = R3D_CreateMaterial(&creatematinfo);
	}

	for (Uint32 i = 0; i < info->mCount; i++) {
		riggedModel->s_model.info[i].indexCount  = info->mInfo[i].indexCount;
		riggedModel->s_model.info[i].indexOffset = info->mInfo[i].indexOffset;
		riggedModel->s_model.info[i].material = materials[info->mInfo[i].materialIdx];
	}

	RGetAllocator()->Deallocate(materials);


	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format              = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	//uavDesc.Buffer.NumElements  = sizeof(R3D_Vertex) * info->vCount / 4;
	uavDesc.Buffer.NumElements  = info->vCount;
	uavDesc.Buffer.Flags        = 0;
	result = DX11_GetDevice()->CreateUnorderedAccessView(riggedModel->s_model.vBuffer->GetHandle(), &uavDesc, &riggedModel->s_srv);


	// Dynamic part

	BufferCreateInfo d_vbufferInfo = {};
	d_vbufferInfo.type      = BUFFER_RESOURCE;
	d_vbufferInfo.access    = BUFFER_GPU_ONLY;
	d_vbufferInfo.usage     = BUFFER_DEFAULT;
	d_vbufferInfo.miscflags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	d_vbufferInfo.length    = sizeof(R3D_Vertex) * info->vCount;
	d_vbufferInfo.stride    = sizeof(R3D_Vertex);

	riggedModel->i_vertex = RG_NEW_CLASS(RGetAllocator(), Buffer)(&d_vbufferInfo);
	riggedModel->i_vertex->SetData(0, d_vbufferInfo.length, info->vertices);

	d_vbufferInfo.length  = sizeof(R3D_Weight) * info->vCount;
	d_vbufferInfo.stride  = sizeof(R3D_Weight);

	riggedModel->i_weight = RG_NEW_CLASS(RGetAllocator(), Buffer)(&d_vbufferInfo);
	riggedModel->i_weight->SetData(0, d_vbufferInfo.length, info->weights);


	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.Flags = 0;
	srvDesc.BufferEx.NumElements = info->vCount;

	result = DX11_GetDevice()->CreateShaderResourceView(riggedModel->i_vertex->GetHandle(), &srvDesc, &riggedModel->i_srv_vtx);
	result = DX11_GetDevice()->CreateShaderResourceView(riggedModel->i_weight->GetHandle(), &srvDesc, &riggedModel->i_srv_wht);

	modelsLoaded += riggedModel->s_model.mCount;

	return riggedModel;
}

void R3D_DestroyRiggedModel(R3D_RiggedModel* hrmdl) {

	FreeMaterials(&hrmdl->s_model);

	RG_DELETE_CLASS(RGetAllocator(), Buffer, hrmdl->s_model.vBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, hrmdl->s_model.iBuffer);
	RGetAllocator()->Deallocate(hrmdl->s_model.info);
	hrmdl->s_srv->Release();

	RG_DELETE_CLASS(RGetAllocator(), Buffer, hrmdl->i_vertex);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, hrmdl->i_weight);
	hrmdl->i_srv_vtx->Release();
	hrmdl->i_srv_wht->Release();
	modelsLoaded -= hrmdl->s_model.mCount;

	alloc_riggedmodels->Deallocate(hrmdl);
}

// Bone buffer

R3D_BoneBuffer* R3D_CreateBoneBuffer(R3DCreateBufferInfo* info) {
	R3D_BoneBuffer* buffer = (R3D_BoneBuffer*)alloc_bonebuffers->Allocate();
	BufferCreateInfo vbufferInfo = {};
	//vbufferInfo.type   = (BufferType)(BUFFER_UNORDERED | BUFFER_RESOURCE);
	vbufferInfo.type   = BUFFER_RESOURCE;
	vbufferInfo.access = BUFFER_CPU_WRITE;
	vbufferInfo.usage  = BUFFER_DYNAMIC;
	vbufferInfo.length = info->len;
	vbufferInfo.stride = sizeof(mat4);
	vbufferInfo.miscflags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffer->bBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format                = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.Flags        = 0;
	srvDesc.BufferEx.NumElements  = info->len / sizeof(mat4);

	HRESULT result = DX11_GetDevice()->CreateShaderResourceView(buffer->bBuffer->GetHandle(), &srvDesc, &buffer->resourceView);

	return buffer;
}

void R3D_DestroyBoneBuffer(R3D_BoneBuffer* hbuff) {
	hbuff->resourceView->Release();
	RG_DELETE_CLASS(RGetAllocator(), Buffer, hbuff->bBuffer);
	alloc_bonebuffers->Deallocate(hbuff);
}

void R3D_UpdateBoneBuffer(R3DUpdateBufferInfo* info) {
	info->handle_bone->bBuffer->SetData(info->offset, info->length, info->data);
}

// Particles

R3D_AtlasHandle* R3D_CreateAtlas(String texture) {
	R3D_AtlasHandle* handle = (R3D_AtlasHandle*)alloc_atlases->Allocate();
	LoaderPushTexture(texture, &handle->texture);
	texturesLoaded++;
	return handle;
}

void R3D_DestroyAtlas(R3D_AtlasHandle* atlas) {
	if (atlas->texture != GetDefaultTexture()) {
		TexturesDelete(atlas->texture);
		texturesLoaded--;
	}
	alloc_atlases->Deallocate(atlas);
}

R3D_ParticleBuffer* R3D_CreateParticleBuffer(R3DCreateBufferInfo* info) {
	R3D_ParticleBuffer* buffer = (R3D_ParticleBuffer*)alloc_particlebuffers->Allocate();
	BufferCreateInfo vbufferInfo = {};
	vbufferInfo.type   = BUFFER_RESOURCE;
	vbufferInfo.access = BUFFER_CPU_WRITE;
	vbufferInfo.usage  = BUFFER_DYNAMIC;
	vbufferInfo.length = info->len;
	vbufferInfo.stride = sizeof(Particle);
	vbufferInfo.miscflags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffer->bBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);

	HRESULT result;

#if 1
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.Flags = 0;
	srvDesc.BufferEx.NumElements = info->len / sizeof(Particle);

	result = DX11_GetDevice()->CreateShaderResourceView(buffer->bBuffer->GetHandle(), &srvDesc, &buffer->resourceView);
#endif
#if 0
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	//uavDesc.Buffer.NumElements  = sizeof(R3D_Vertex) * info->vCount / 4;
	uavDesc.Buffer.NumElements = info->len / sizeof(Particle);
	uavDesc.Buffer.Flags = 0;
	result = DX11_GetDevice()->CreateUnorderedAccessView(buffer->bBuffer->GetHandle(), &uavDesc, &buffer->uav);
#endif
	return buffer;
}

void R3D_DestroyParticleBuffer(R3D_ParticleBuffer* hbuff) {
	hbuff->resourceView->Release();
	//hbuff->uav->Release();
	RG_DELETE_CLASS(RGetAllocator(), Buffer, hbuff->bBuffer);
	alloc_particlebuffers->Deallocate(hbuff);
}

void R3D_UpdateParticleBuffer(R3DUpdateBufferInfo* info) {
	info->handle_particle->bBuffer->SetData(info->offset, info->length, info->data);
}

void R3D_PushLightSource(R3D_LightSource* light) {
	R3D_LightSource* src = (R3D_LightSource*)alloc_lights->Allocate(sizeof(R3D_LightSource));
	SDL_memcpy(src, light, sizeof(R3D_LightSource));
	lightQueue->Push(src);
}

void R3D_PushModel(R3D_PushModelInfo* info) {
	mat4* mat = (mat4*)alloc_matrices->Allocate(sizeof(mat4));
	SDL_memcpy(mat, &info->matrix, sizeof(mat4));

	if (info->handle_static->type == R_MODEL_STATIC) {
		staticQueue->Push(info->handle);
		staticQueue->Push(mat);
	} else {
		riggedQueue->Push(info->handle);
		riggedQueue->Push(info->handle_bonebuffer);
		riggedQueue->Push(mat);
	}
}

void R3D_SetCamera(R3D_CameraInfo* info) {
	cam_proj = info->projection;
	cam_pos  = info->position;
	cam_rot  = info->rotation;
	mat4_view(&cam_view, info->position, info->rotation);
}

////////////////////////////////////////////////


static inline DXGI_FORMAT GetIndexType(IndexType type) {
	switch (type) {
		case RG_INDEX_U8:  return DXGI_FORMAT_R8_UINT;
		case RG_INDEX_U16: return DXGI_FORMAT_R16_UINT;
		case RG_INDEX_U32: return DXGI_FORMAT_R32_UINT;
		default:           return DXGI_FORMAT_R8_UINT;
	}
}

#define SKYBOX_SCALE 250

static void DrawStaticModel(R3D_StaticModel* mdl, mat4* matrix, Bool useMaterial, Bool useOnlyColor) {
	matrixBuffer.model = *matrix;
	mBuffer->SetData(0, sizeof(MatrixBuffer), &matrixBuffer);

	UINT stride = sizeof(R3D_Vertex);
	UINT offset = 0;
	ID3D11Buffer* vbuffer = mdl->vBuffer->GetHandle();
	DX11_GetContext()->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	DX11_GetContext()->IASetIndexBuffer(mdl->iBuffer->GetHandle(), GetIndexType(mdl->iType), 0);
	DX11_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	R3D_Material* current_mat = NULL;


	constBuffer.color.r = 1;
	constBuffer.color.g = 1;
	constBuffer.color.b = 1;
	constBuffer.color.a = 1;

	cBuffer->SetData(0, sizeof(ConstBuffer), &constBuffer);
	ID3D11Buffer* conBuffer = cBuffer->GetHandle();
	DX11_GetContext()->PSSetConstantBuffers(0, 1, &conBuffer);

	//Uint32 idx = 0;
	for (Uint32 i = 0; i < mdl->mCount; i++) {
	//Uint32 count = SDL_min(mdl->mCount, 100);
	//for (Uint32 i = 0; i < count; i++) {

		R3D_MeshInfo* minfo = &mdl->info[i];
		R3D_Material* mat = minfo->material;

		if (useMaterial && current_mat != mat) {

			// Bind material
			constBuffer.color.r = mat->color.r;
			constBuffer.color.g = mat->color.g;
			constBuffer.color.b = mat->color.b;
			constBuffer.color.a = 1;

			cBuffer->SetData(0, sizeof(ConstBuffer), &constBuffer);
			ID3D11Buffer* conBuffer = cBuffer->GetHandle();
			DX11_GetContext()->PSSetConstantBuffers(0, 1, &conBuffer);

			mat->albedo->Bind(0);
			if (!useOnlyColor) {
				mat->normal->Bind(1);
				mat->pbr->Bind(2);
			}

			current_mat = mat;
		}


		DX11_GetContext()->DrawIndexed(minfo->indexCount, minfo->indexOffset, 0);
		drawCalls++;
		//idx += minfo->indexCount;

	}
}

static void DrawSkybox() {
	mat4_model(&matrixBuffer.model, cam_pos, { 0, 0, 0 }, { SKYBOX_SCALE, SKYBOX_SCALE, SKYBOX_SCALE });
	mBuffer->SetData(0, sizeof(MatrixBuffer), &matrixBuffer);

	UINT stride = sizeof(R3D_Vertex);
	UINT offset = 0;
	ID3D11Buffer* vbuffer = skyboxv->GetHandle();
	DX11_GetContext()->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	DX11_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//vec3    sun_dir;
	//Float32 m_turbidity;  -> color

	constBuffer.color.xyz = (*GetSunPosition()).normalize();
	constBuffer.color.a   = GetAtmosphereTurbidity();
	constBuffer.camerapos = cam_pos;

	cBuffer->SetData(0, sizeof(ConstBuffer), &constBuffer);
	ID3D11Buffer* conBuffer = cBuffer->GetHandle();
	DX11_GetContext()->PSSetConstantBuffers(0, 1, &conBuffer);

	GetDefaultTexture()->Bind(0);
	GetDefaultTexture()->Bind(1);
	GetDefaultTexture()->Bind(2);

	DX11_GetContext()->Draw(36, 0);
	drawCalls++;
}

static void DoSkeletonCalculation() {
	RQueue* squeue = GetStaticQueue();
	RQueue* rqueue = GetRiggedQueue();
	Uint32 rsize = rqueue->Size() / 3;

	ID3D11DeviceContext* ctx = DX11_GetContext();

	skeletonShader->Bind();

	ID3D11ShaderResourceView* views[3] = {};
	ID3D11UnorderedAccessView* uav = NULL;

	for (Uint32 i = 0; i < rsize; i++) {

		R3D_RiggedModel* mdl = (R3D_RiggedModel*)rqueue->Next(); // NOT USED
		R3D_BoneBuffer* buff = (R3D_BoneBuffer*)rqueue->Next();
		mat4* matrix = (mat4*)rqueue->Next();

		/////////
		// TMP //
		/////////
		if (matrix->m33 < 1) { continue; } // Culled, skip this one
		/////////

		views[0] = buff->resourceView; // Bone buffer
		views[1] = mdl->i_srv_vtx;     // Input vertex data
		views[2] = mdl->i_srv_wht;     // Input weight data
		uav = mdl->s_srv;         // Output vertex data

		ctx->CSSetShaderResources(0, 3, views);
		ctx->CSSetUnorderedAccessViews(0, 1, &uav, NULL);

		skeletonShader->Dispatch({ 100000, 1, 1 }); // !!!WARNING!!! MAX 10000 Vertices
		dispatchCalls++;
	}

	uav = NULL;
	ctx->CSSetUnorderedAccessViews(0, 1, &uav, NULL);
	rqueue->Reset();
}

static void DoShadowmapPass() {
	RQueue* squeue = GetStaticQueue();
	RQueue* rqueue = GetRiggedQueue();
	Uint32 ssize = squeue->Size() / 2;
	Uint32 rsize = rqueue->Size() / 3;

	ID3D11DeviceContext* ctx = DX11_GetContext();

	BindShadowBuffer();

	// Global light projection (ortho matrix) * Global light direction (rotation matrix)
	//matrixBuffer.viewproj = *GetCameraProjection() * *GetCameraView();
	matrixBuffer.viewproj = *GetLightMatrix();
	shadowshader->Bind();

	ID3D11Buffer* matBuffer = mBuffer->GetHandle();
	ctx->VSSetConstantBuffers(0, 1, &matBuffer);

	for (Uint32 i = 0; i < ssize; i++) {
		R3D_StaticModel* mdl = (R3D_StaticModel*)squeue->Next();
		mat4* matrix = (mat4*)squeue->Next();
		if (matrix->m33 < 1) { matrix->m33 = 1; }
		DrawStaticModel(mdl, matrix, true, true);
	}

	for (Uint32 i = 0; i < rsize; i++) {
		R3D_RiggedModel* mdl = (R3D_RiggedModel*)rqueue->Next();
		R3D_BoneBuffer* buff = (R3D_BoneBuffer*)rqueue->Next();  // NOT USED
		mat4* matrix = (mat4*)rqueue->Next();
		if (matrix->m33 < 1) { matrix->m33 = 1; }
		DrawStaticModel(&mdl->s_model, matrix, true, true);
	}

	squeue->Reset();
	rqueue->Reset();
}

static void DoGBufferPass() {

	RQueue* squeue = GetStaticQueue();
	RQueue* rqueue = GetRiggedQueue();
	Uint32 rsize = rqueue->Size() / 3;

	ID3D11DeviceContext* ctx = DX11_GetContext();

	BindGBuffer();

	matrixBuffer.viewproj = *GetCameraProjection() * *GetCameraView();

	shader->Bind();
	ID3D11Buffer* matBuffer = mBuffer->GetHandle();
	ctx->VSSetConstantBuffers(0, 1, &matBuffer);

	Uint32 ssize = squeue->Size() / 2;
	for (Uint32 i = 0; i < ssize; i++) {
		R3D_StaticModel* mdl = (R3D_StaticModel*)squeue->Next();
		mat4* matrix = (mat4*)squeue->Next();
		if (matrix->m33 < 1) { continue; } // Culled, skip this one
		DrawStaticModel(mdl, matrix, true, false);
	}

	for (Uint32 i = 0; i < rsize; i++) {
		R3D_RiggedModel* mdl = (R3D_RiggedModel*)rqueue->Next();
		R3D_BoneBuffer* buff = (R3D_BoneBuffer*)rqueue->Next();  // NOT USED
		mat4* matrix = (mat4*)rqueue->Next();
		if (matrix->m33 < 1) { continue; } // Culled, skip this one
		DrawStaticModel(&mdl->s_model, matrix, true, false);
	}

	squeue->Reset();
	rqueue->Reset();

	skyshader->Bind();
	ctx->VSSetConstantBuffers(0, 1, &matBuffer);

	DrawSkybox();
}

void R3D_StartRenderTask(R3D_RenderTaskInfo* info) {
	drawCalls     = 0;
	dispatchCalls = 0;

	Float32 blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	DX11_GetContext()->OMSetBlendState(NULL, blendFactor, 0xffffffff);

	SetLightDescription(info->globallight);

	DoSkeletonCalculation(); // Calculate skeletons
	DoShadowmapPass();       // Global shadowmap pass
	DoGBufferPass();         // Geometry pass

	GetStaticQueue()->Clear();
	GetRiggedQueue()->Clear();

	RenderParticles(); // Particle pass

	DoLightpass(lightQueue); // Light pass
	DoPostprocess();         // Postprocess

	alloc_matrices->Deallocate();
	alloc_lights->Deallocate();
}