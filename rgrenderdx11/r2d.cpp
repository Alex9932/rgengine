#include "r2d.h"
#include <rshared.h>
#include <filesystem.h>
#include <allocator.h>
#include "dx11.h"
#include "rgrenderdx11.h"
#include "shader.h"

static RenderTarget           rendertarget = {};
static ID3D11RasterizerState* rasterState  = NULL;

static ID3D11BlendState*      blendState   = NULL;

static ivec2 viewport;

static Shader* shader;

void InitializeR2D(ivec2* size) {
	viewport = *size;
	DX11_MakeRenderTarget(&rendertarget, size, DXGI_FORMAT_R16G16B16A16_FLOAT);

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	DX11_GetDevice()->CreateRasterizerState(&rasterDesc, &rasterState);

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	DX11_GetDevice()->CreateBlendState(&blendDesc, &blendState);

	InputDescription staticDescriptions[3] = {};
	staticDescriptions[0].name = "POSITION";
	staticDescriptions[0].inputSlot = 0;
	staticDescriptions[0].format = INPUT_R32G32_FLOAT;
	staticDescriptions[1].name = "VPOS";
	staticDescriptions[1].inputSlot = 0;
	staticDescriptions[1].format = INPUT_R32G32_FLOAT;
	staticDescriptions[2].name = "COLOR";
	staticDescriptions[2].inputSlot = 0;
	staticDescriptions[2].format = INPUT_R32G32B32A32_FLOAT;
	PipelineDescription staticDescription = {};
	staticDescription.inputCount = 3;
	staticDescription.descriptions = staticDescriptions;

	char vs[128];
	char ps[128];
	Engine::GetPath(vs, 128, RG_PATH_SYSTEM, "shadersdx11/r2d.vs");
	Engine::GetPath(ps, 128, RG_PATH_SYSTEM, "shadersdx11/r2d.ps");
	shader = RG_NEW_CLASS(RGetAllocator(), Shader)(&staticDescription, vs, ps, false);

}

void DestroyR2D() {
	DX11_FreeRenderTarget(&rendertarget);
	rasterState->Release();
	blendState->Release();
	RG_DELETE_CLASS(RGetAllocator(), Shader, shader);
}

void ResizeR2D(ivec2* wndSize) {
	viewport = *wndSize;
	DX11_FreeRenderTarget(&rendertarget);
	DX11_MakeRenderTarget(&rendertarget, wndSize, DXGI_FORMAT_R16G16B16A16_FLOAT);
}

void ReloadShadersR2D() {

}

ID3D11ShaderResourceView* R2DGetOuputTexture() {
	return rendertarget.resView;
}

R2D_Buffer* R2D_CreateBuffer(R2DCreateBufferInfo* info) {
	return NULL;
}

void R2D_DestroyBuffer(R2D_Buffer* buffer) {

}

void R2D_BufferData(R2DBufferDataInfo* info) {

}

R2D_Texture* R2D_CreateTexture(R2DCreateTextureInfo* info) {
	return NULL;
}

void R2D_DestroyTexture(R2D_Texture* texture) {

}

void R2D_TextureData(R2DTextureDataInfo* info) {

}

void R2D_PushMatrix(mat4* matrix) {

}

mat4* R2D_PopMatrix() {
	return NULL;
}

void R2D_ResetStack() {

}

void R2D_Begin() {
	ID3D11DeviceContext* ctx = DX11_GetContext();

	ctx->OMSetRenderTargets(1, &rendertarget.rtView, NULL);
	ctx->RSSetState(rasterState);

	Float32 blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	ctx->OMSetBlendState(blendState, blendFactor, 0xffffffff);

	DX11_SetViewport(viewport.x, viewport.y);
	Float32 clearcolor[] = { 1, 1, 1, 0.0f };
	ctx->ClearRenderTargetView(rendertarget.rtView, clearcolor);



}

void R2D_Bind(R2DBindInfo* info) {

}

void R2D_Draw(R2DDrawInfo* info) {

}