#define DLL_EXPORT
#include "r3d.h"
#include "dx11.h"
#include "rgrenderdx11.h"
#include <rshared.h>
#include <filesystem.h>

#include <rgmath.h>

#include "shader.h"
#include "gbuffer.h"
#include "lightpass.h"

#include "loader.h"

#define R_MATERIALS_COUNT 4096
#define R_MODELS_COUNT    4096
#define R_MAX_MODELS      4096

using namespace Engine;

static PoolAllocator*   alloc_materials;
static PoolAllocator*   alloc_staticmodels;
static PoolAllocator*   alloc_riggedmodels;
static LinearAllocator* alloc_matrices;
static PoolAllocator*   alloc_bonebuffers;

static RQueue* staticQueue;
static RQueue* riggedQueue;

struct MatrixBuffer {
	mat4 viewproj;
	mat4 model;
};

struct ConstBuffer {
	vec4 color;
};

static Shader* shader;
static Buffer* mBuffer;
static Buffer* cBuffer;

static MatrixBuffer matrixBuffer;
static ConstBuffer  constBuffer;
static Float32      time = 0;

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

void InitializeR3D(ivec2* size) {
	alloc_materials    = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_MaterialPool", R_MATERIALS_COUNT, sizeof(R3D_Material));
	alloc_staticmodels = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_StaticModelPool", R_MODELS_COUNT, sizeof(R3D_StaticModel));
	alloc_riggedmodels = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_RiggedModelPool", R_MODELS_COUNT, sizeof(R3D_RiggedModel));
	alloc_matrices     = RG_NEW_CLASS(RGetAllocator(), LinearAllocator)("R_MatrixPool", sizeof(mat4) * R_MODELS_COUNT * 2);
	alloc_bonebuffers  = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_BoneBuffersPool", R_MODELS_COUNT, sizeof(R3D_BoneBuffer));
	//alloc_matrices     = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_MatrixPool", R_MODELS_COUNT * 2, sizeof(mat4));
	staticQueue = RG_NEW_CLASS(RGetAllocator(), RQueue)(R_MAX_MODELS * 2);
	riggedQueue = RG_NEW_CLASS(RGetAllocator(), RQueue)(R_MAX_MODELS * 3);

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
	Engine::GetPath(gbuff_ps, 128, RG_PATH_SYSTEM, "shadersdx11/gbuffer.ps");
	shader = RG_NEW_CLASS(RGetAllocator(), Shader)(&staticDescription, gbuff_vs, gbuff_ps, false);

	BufferCreateInfo bInfo = {};
	bInfo.access = BUFFER_CPU_WRITE;
	bInfo.usage = BUFFER_DYNAMIC;
	bInfo.type = BUFFER_CONSTANT;
	bInfo.length = sizeof(MatrixBuffer);
	mBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&bInfo);
	bInfo.length = sizeof(ConstBuffer);
	cBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&bInfo);

	CreateGBuffer(size);
	CreateLightpass(size);
}

void DestroyR3D() {

	DestroyLightpass();
	DestroyGBuffer();

	RG_DELETE_CLASS(RGetAllocator(), ComputeShader, skeletonShader);

	RG_DELETE_CLASS(RGetAllocator(), Shader, shader);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, mBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, cBuffer);

	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_materials);
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_staticmodels);
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_riggedmodels);
	RG_DELETE_CLASS(RGetAllocator(), LinearAllocator, alloc_matrices);
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_bonebuffers);
	//RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_matrices);
	RG_DELETE_CLASS(RGetAllocator(), RQueue, staticQueue);
	RG_DELETE_CLASS(RGetAllocator(), RQueue, riggedQueue);
}

void ResizeR3D(ivec2* wndSize) {

	ResizeGbuffer(wndSize);
	ResizeLightpass(wndSize);

}

RQueue* GetStaticQueue() { return staticQueue; }
RQueue* GetRiggedQueue() { return riggedQueue; }

mat4* GetCameraProjection() { return &cam_proj; }
mat4* GetCameraView()       { return &cam_view; }
vec3* GetCameraPosition()   { return &cam_pos; }
vec3* GetCameraRotation()   { return &cam_rot; }

LinearAllocator* GetMatrixAllocator() { return alloc_matrices; }

void GetR3DStats(R3DStats* stats) {
	stats->texturesLoaded = texturesLoaded;
	stats->modelsLoaded   = modelsLoaded;
	stats->drawCalls      = drawCalls;
	stats->dispatchCalls  = dispatchCalls;
}

////// PUBLIC API //////

R3D_Material* R3D_CreateMaterial(R3DCreateMaterialInfo* info) {
	R3D_Material* material = (R3D_Material*)alloc_materials->Allocate();

	rgLogInfo(RG_LOG_RENDER, "Material: %p", material);

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
	if (hmat->albedo != defaultTexture) { RG_DELETE_CLASS(RGetAllocator(), Texture, hmat->albedo); texturesLoaded--; }
	if (hmat->normal != defaultTexture) { RG_DELETE_CLASS(RGetAllocator(), Texture, hmat->normal); texturesLoaded--; }
	if (hmat->pbr != defaultTexture)    { RG_DELETE_CLASS(RGetAllocator(), Texture, hmat->pbr);    texturesLoaded--; }

	alloc_materials->Deallocate(hmat);
}

R3D_StaticModel* R3D_CreateStaticModel(R3DCreateStaticModelInfo* info) {
	R3D_StaticModel* staticModel = (R3D_StaticModel*)alloc_staticmodels->Allocate();
	staticModel->type = R_MODEL_STATIC;

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

	Uint32 l = sizeof(R3D_MeshInfo) * info->mCount;
	staticModel->info = (R3D_MeshInfo*)RGetAllocator()->Allocate(l);
	SDL_memcpy(staticModel->info, info->info, l);

	modelsLoaded += staticModel->mCount;

	return staticModel;
}

void R3D_DestroyStaticModel(R3D_StaticModel* hsmdl) {
	rgLogInfo(RG_LOG_RENDER, "Free static model: %p", hsmdl);

	//for (Uint32 i = 0; i < hsmdl->mCount; i++) {
	//	R3D_DestroyMaterial(hsmdl->info[i].material);
	//}

	RG_DELETE_CLASS(RGetAllocator(), Buffer, hsmdl->vBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, hsmdl->iBuffer);
	RGetAllocator()->Deallocate(hsmdl->info);

	modelsLoaded += hsmdl->mCount;

	alloc_staticmodels->Deallocate(hsmdl);
}

R3D_RiggedModel* R3D_CreateRiggedModel(R3DCreateRiggedModelInfo* info) {
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

	Uint32 l = sizeof(R3D_MeshInfo) * info->mCount;
	riggedModel->s_model.info = (R3D_MeshInfo*)RGetAllocator()->Allocate(l);
	SDL_memcpy(riggedModel->s_model.info, info->info, l);

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

// !!! Not tested !!!
R3D_BoneBuffer* R3D_CreateBoneBuffer(R3DCreateBoneBufferInfo* info) {
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

#if 0
	mat4 data[1024];
	for (Uint32 i = 0; i < 1024; i++) {
		data[i] = MAT4_IDENTITY();
	}
	buffer->bBuffer->SetData(0, info->len, data);
#endif

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format                = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.Flags        = 0;
	srvDesc.BufferEx.NumElements  = info->len / sizeof(mat4);

	HRESULT result = DX11_GetDevice()->CreateShaderResourceView(buffer->bBuffer->GetHandle(), &srvDesc, &buffer->resourceView);

	return buffer;
}

void R3D_DestroyBoneBuffer(R3D_BoneBuffer* hbuff) {
	RG_DELETE_CLASS(RGetAllocator(), Buffer, hbuff->bBuffer);
	alloc_bonebuffers->Deallocate(hbuff);
}

void R3D_UpdateBoneBuffer(R3DBoneBufferUpdateInfo* info) {
	info->handle->bBuffer->SetData(info->offset, info->length, info->data);
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

static void DrawStaticModel(R3D_StaticModel* mdl, mat4* matrix) {
	matrixBuffer.model = *matrix;
	mBuffer->SetData(0, sizeof(MatrixBuffer), &matrixBuffer);

	UINT stride = sizeof(R3D_Vertex);
	UINT offset = 0;
	ID3D11Buffer* vbuffer = mdl->vBuffer->GetHandle();
	DX11_GetContext()->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	DX11_GetContext()->IASetIndexBuffer(mdl->iBuffer->GetHandle(), GetIndexType(mdl->iType), 0);
	DX11_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Uint32 idx = 0;
	for (Uint32 i = 0; i < mdl->mCount; i++) {

		R3D_MeshInfo* minfo = &mdl->info[i];
		R3D_Material* mat = minfo->material;

		constBuffer.color.r = mat->color.r;
		constBuffer.color.g = mat->color.g;
		constBuffer.color.b = mat->color.b;
		constBuffer.color.a = 1;

		cBuffer->SetData(0, sizeof(ConstBuffer), &constBuffer);
		ID3D11Buffer* conBuffer = cBuffer->GetHandle();
		DX11_GetContext()->PSSetConstantBuffers(0, 1, &conBuffer);

		mat->albedo->Bind(0);
		mat->normal->Bind(1);
		mat->pbr->Bind(2);
		DX11_GetContext()->DrawIndexed(minfo->indexCount, idx, 0);
		drawCalls++;
		idx += minfo->indexCount;

	}
}

void R3D_StartRenderTask(void* RESERVERD_PTR) {
	drawCalls     = 0;
	dispatchCalls = 0;

	RQueue* squeue = GetStaticQueue();
	RQueue* rqueue = GetRiggedQueue();

	ID3D11DeviceContext* ctx = DX11_GetContext();

	// Calculate skeletons
	Uint32 rsize = rqueue->Size() / 3;


	skeletonShader->Bind();

	ID3D11ShaderResourceView* views[3];
	ID3D11UnorderedAccessView* uav;

	for (Uint32 i = 0; i < rsize; i++) {

		R3D_RiggedModel* mdl = (R3D_RiggedModel*)rqueue->Next(); // NOT USED
		R3D_BoneBuffer* buff = (R3D_BoneBuffer*)rqueue->Next();
		mat4* matrix = (mat4*)rqueue->Next();                    // NOT USED

		views[0] = buff->resourceView; // Bone buffer
		views[1] = mdl->i_srv_vtx;     // Input vertex data
		views[2] = mdl->i_srv_wht;     // Input weight data
		uav      = mdl->s_srv;         // Output vertex data


		ctx->CSSetShaderResources(0, 3, views);
		ctx->CSSetUnorderedAccessViews(0, 1, &uav, NULL);

		skeletonShader->Dispatch({ 10000, 1, 1 });
		dispatchCalls++;
	}

	uav = NULL;
	ctx->CSSetUnorderedAccessViews(0, 1, &uav, NULL);
	rqueue->Reset();

	// Geometry pass

	BindGBuffer();

	matrixBuffer.viewproj = *GetCameraProjection() * *GetCameraView();

	shader->Bind();
	ID3D11Buffer* matBuffer = mBuffer->GetHandle();
	ctx->VSSetConstantBuffers(0, 1, &matBuffer);

	Uint32 ssize = squeue->Size() / 2;
	for (Uint32 i = 0; i < ssize; i++) {
		R3D_StaticModel* mdl = (R3D_StaticModel*)squeue->Next();
		mat4* matrix = (mat4*)squeue->Next();
		DrawStaticModel(mdl, matrix);
	}

	for (Uint32 i = 0; i < rsize; i++) {
		R3D_RiggedModel* mdl = (R3D_RiggedModel*)rqueue->Next();
		R3D_BoneBuffer* buff = (R3D_BoneBuffer*)rqueue->Next();  // NOT USED
		mat4* matrix = (mat4*)rqueue->Next();
		DrawStaticModel(&mdl->s_model, matrix);
	}

	squeue->Clear();
	rqueue->Clear();
	GetMatrixAllocator()->Deallocate();

	// Light pass

	DoLightpass();


}