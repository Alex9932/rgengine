#include "r2d.h"
#include <rshared.h>
#include <filesystem.h>
#include <allocator.h>
#include "dx11.h"
#include "rgrenderdx11.h"
#include "shader.h"

#include "stack.h"
#include <rgmath.h>

#include "loader.h"

#include <allocator.h>

#define R_BUFFERS_COUNT 1024

using namespace Engine;

static RenderTarget           rendertarget = {};
static ID3D11RasterizerState* rasterState  = NULL;

static ID3D11BlendState*      blendState   = NULL;

static ivec2 viewport;

static Shader* shader;

static Stack*  stack;
static mat4    local_transform;

static PoolAllocator* alloc_buffers;

static void LoadShaders() {
	InputDescription staticDescriptions[3] = {};
	staticDescriptions[0].name      = "POSITION";
	staticDescriptions[0].inputSlot = 0;
	staticDescriptions[0].format    = INPUT_R32G32_FLOAT;
	staticDescriptions[1].name      = "VPOS";
	staticDescriptions[1].inputSlot = 0;
	staticDescriptions[1].format    = INPUT_R32G32_FLOAT;
	staticDescriptions[2].name      = "COLOR";
	staticDescriptions[2].inputSlot = 0;
	staticDescriptions[2].format    = INPUT_R32G32B32A32_FLOAT;

	PipelineDescription staticDescription = {};
	staticDescription.inputCount   = 3;
	staticDescription.descriptions = staticDescriptions;

	char vs[128];
	char ps[128];
	GetPath(vs, 128, RG_PATH_SYSTEM, "shadersdx11/r2d.vs");
	GetPath(ps, 128, RG_PATH_SYSTEM, "shadersdx11/r2d.ps");
	shader = RG_NEW_CLASS(RGetAllocator(), Shader)(&staticDescription, vs, ps, false);

	alloc_buffers = RG_NEW_CLASS(RGetAllocator(), PoolAllocator)("R_BuffersPool", R_BUFFERS_COUNT, sizeof(R2D_Buffer));
	stack = RG_NEW_CLASS(RGetAllocator(), Stack)(RGetAllocator(), sizeof(mat4), 256);
	local_transform = MAT4_IDENTITY();
}

static void FreeShaders() {
	RG_DELETE_CLASS(RGetAllocator(), PoolAllocator, alloc_buffers);
	RG_DELETE_CLASS(RGetAllocator(), Shader, shader);
	RG_DELETE_CLASS(RGetAllocator(), Stack, stack);
}

#if 0

static void printmat(mat4* mat) {
	if (mat == NULL) { rgLogInfo(RG_LOG_RENDER, "NULL MATRIX"); return; }

	rgLogInfo(RG_LOG_RENDER, "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n---",
		mat->m00, mat->m01, mat->m02, mat->m03,
		mat->m10, mat->m11, mat->m12, mat->m13,
		mat->m20, mat->m21, mat->m22, mat->m23,
		mat->m30, mat->m31, mat->m32, mat->m33
	);
}

#endif

void InitializeR2D(ivec2* size) {
	viewport = *size;
	DX11_MakeRenderTarget(&rendertarget, size, DXGI_FORMAT_R16G16B16A16_FLOAT);

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode              = D3D11_CULL_NONE;
	rasterDesc.DepthBias             = 0;
	rasterDesc.DepthBiasClamp        = 0.0f;
	rasterDesc.DepthClipEnable       = true;
	rasterDesc.FillMode              = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable     = false;
	rasterDesc.ScissorEnable         = false;
	rasterDesc.SlopeScaledDepthBias  = 0.0f;
	DX11_GetDevice()->CreateRasterizerState(&rasterDesc, &rasterState);

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable                 = false;
	blendDesc.IndependentBlendEnable                = false;
	blendDesc.RenderTarget[0].BlendEnable           = false;
	//blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_DEST_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	DX11_GetDevice()->CreateBlendState(&blendDesc, &blendState);

	LoadShaders();
#if 0
	rgLogInfo(RG_LOG_RENDER, "Stack test");

	mat4 m0 = {};
	mat4 m1 = {};
	mat4 m2 = {};
	mat4 m3 = {};
	mat4 m4 = {};
	mat4 m5 = {};

	mat4_rotatex(&m0, 0);
	mat4_rotatey(&m1, 0);
	mat4_rotatez(&m2, 0);
	mat4_rotatex(&m3, 3.1415);
	mat4_rotatey(&m4, 3.1415);
	mat4_rotatez(&m5, 3.1415);

	Stack stack(RGetAllocator(), sizeof(mat4), 256);

	printmat(&m0);
	printmat(&m1);
	printmat(&m2);
	printmat(&m3);
	printmat(&m4);
	printmat(&m5);

	stack.Push(&m0);
	stack.Push(&m1);
	stack.Push(&m2);
	stack.Push(&m3);
	stack.Push(&m4);
	stack.Push(&m5);

	printmat((mat4*)stack.Pop());
	printmat((mat4*)stack.Pop());
	printmat((mat4*)stack.Pop());
	printmat((mat4*)stack.Pop());
	printmat((mat4*)stack.Pop());
	printmat((mat4*)stack.Pop());
	printmat((mat4*)stack.Pop());
#endif
}

void DestroyR2D() {
	DX11_FreeRenderTarget(&rendertarget);
	rasterState->Release();
	blendState->Release();

	FreeShaders();
}

void ResizeR2D(ivec2* wndSize) {
	viewport = *wndSize;
	DX11_FreeRenderTarget(&rendertarget);
	DX11_MakeRenderTarget(&rendertarget, wndSize, DXGI_FORMAT_R16G16B16A16_FLOAT);
}

void ReloadShadersR2D() {
	FreeShaders();
	LoadShaders();
}

ID3D11ShaderResourceView* R2DGetOuputTexture() {
	return rendertarget.resView;
}

R2D_Buffer* R2D_CreateBuffer(R2DCreateBufferInfo* info) {
	R2D_Buffer* buffer = (R2D_Buffer*)alloc_buffers->Allocate();

	BufferCreateInfo vbufferInfo = {};
	vbufferInfo.type   = BUFFER_VERTEX;
	vbufferInfo.access = BUFFER_GPU_ONLY;
	vbufferInfo.usage  = BUFFER_DEFAULT;
	vbufferInfo.length = info->length * sizeof(R2D_Vertex);
	buffer->buffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);
	buffer->buffer->SetData(0, vbufferInfo.length, info->initial_data);
	buffer->vertices = info->length;

	return buffer;
}

void R2D_DestroyBuffer(R2D_Buffer* buffer) {
	RG_DELETE_CLASS(RGetAllocator(), Buffer, buffer->buffer);
	alloc_buffers->Deallocate(buffer);
}

void R2D_BufferData(R2DBufferDataInfo* info) {
	info->buffer->buffer->SetData(info->offset, info->length, info->data);
}

R2D_Texture* R2D_CreateTexture(R2DCreateTextureInfo* info) {
	return NULL;
}

void R2D_DestroyTexture(R2D_Texture* texture) {

}

void R2D_TextureData(R2DTextureDataInfo* info) {

}

void R2D_PushMatrix(mat4* matrix) {
	stack->Push(matrix);
	mat4 transform = local_transform * *matrix;
	local_transform = transform;
}

mat4* R2D_PopMatrix() {
	mat4* matrix = (mat4*)stack->Pop();

	// Recalculate transform
	Uint32 size = stack->GetSize();
	local_transform = MAT4_IDENTITY();
	for (Uint32 i = 0; i < size; i++) {
		mat4* mat = (mat4*)stack->Get(i);
		local_transform = local_transform * *mat;
	}

	return matrix;
}

void R2D_ResetStack() {
	stack->Reset();
	local_transform = MAT4_IDENTITY();
}

void R2D_Begin() {
	ID3D11DeviceContext* ctx = DX11_GetContext();

	ctx->OMSetRenderTargets(1, &rendertarget.rtView, NULL);
	ctx->RSSetState(rasterState);

	Float32 blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	ctx->OMSetBlendState(blendState, blendFactor, 0xffffffff);

	DX11_SetViewport(viewport.x, viewport.y);
	Float32 clearcolor[] = { 0, 0, 0, 0.0f };
	ctx->ClearRenderTargetView(rendertarget.rtView, clearcolor);

	shader->Bind();
}

void R2D_Bind(R2DBindInfo* info) {

	ID3D11DeviceContext* ctx = DX11_GetContext();

	if (info->texture) {
		info->texture->texture->Bind(0);
	} else {
		GetDefaultTexture()->Bind(0);
	}

	if (info->buffer) {
		
		ID3D11Buffer* vbuffer = info->buffer->buffer->GetHandle();
		UINT stride = sizeof(R2D_Vertex);
		UINT offset = 0;

		ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
}

void R2D_Draw(R2DDrawInfo* info) {
	ID3D11DeviceContext* ctx = DX11_GetContext();
	ctx->Draw(info->count, info->offset);
}