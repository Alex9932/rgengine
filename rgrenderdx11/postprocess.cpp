#include "postprocess.h"
#include "rgrenderdx11.h"
#include "shader.h"
#include "buffer.h"

#include "gbuffer.h"
#include "lightpass.h"

#include <filesystem.h>
#include <allocator.h>

static RenderTarget lightpass;
static Shader*      lightShader;

static RenderTarget aberration;
static Shader*      aberrationShader;

static RenderTarget tonemapping;
static Shader*      tonemappingShader;

static ID3D11RasterizerState* rasterState = NULL;

static Buffer* quadbuffer = NULL;
static Float32 quadvertices[] = {
	-1,  1,  1,  1,  1, -1,
	 1, -1, -1, -1, -1,  1
};

static void _CreateBuffers(ivec2* size) {
	DX11_MakeRenderTarget(&lightpass, size, DXGI_FORMAT_R16G16B16A16_FLOAT);
	DX11_MakeRenderTarget(&aberration, size, DXGI_FORMAT_R16G16B16A16_FLOAT);
	DX11_MakeRenderTarget(&tonemapping, size, DXGI_FORMAT_R16G16B16A16_FLOAT);
}

static void _DestroyBuffers() {
	DX11_FreeRenderTarget(&lightpass);
	DX11_FreeRenderTarget(&aberration);
	DX11_FreeRenderTarget(&tonemapping);
}

static Shader* _MakeShader(String vs, String ps) {
	InputDescription inputDescription = {};
	inputDescription.name = "POSITION";
	inputDescription.inputSlot = 0;
	inputDescription.format = INPUT_R32G32_FLOAT;
	SamplerDescription sampler = {};
	sampler.u = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.v = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.w = D3D11_TEXTURE_ADDRESS_CLAMP;
	PipelineDescription pipelineDescription = {};
	pipelineDescription.sampler      = &sampler;
	pipelineDescription.inputCount   = 1;
	pipelineDescription.descriptions = &inputDescription;

	char vpath[128];
	char ppath[128];
	char vs_path[128];
	char ps_path[128];
	SDL_snprintf(vpath, 128, "shadersdx11/fx/%s", vs);
	SDL_snprintf(ppath, 128, "shadersdx11/fx/%s", ps);
	Engine::GetPath(vs_path, 128, RG_PATH_SYSTEM, vpath);
	Engine::GetPath(ps_path, 128, RG_PATH_SYSTEM, ppath);
	return RG_NEW_CLASS(RGetAllocator(), Shader)(&pipelineDescription, vs_path, ps_path, false);
}

void CreateFX(ivec2* size) {
	_CreateBuffers(size);

	// Raster state
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

	// Vertex buffer
	BufferCreateInfo vbufferInfo = {};
	vbufferInfo.type = BUFFER_VERTEX;
	vbufferInfo.access = BUFFER_GPU_ONLY;
	vbufferInfo.usage = BUFFER_DEFAULT;
	vbufferInfo.length = sizeof(quadvertices);
	vbufferInfo.initialData = quadvertices;
	quadbuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);

	lightShader       = _MakeShader("base.vs", "light.ps");
	aberrationShader  = _MakeShader("base.vs", "aberration.ps");
	tonemappingShader = _MakeShader("base.vs", "tonemapping.ps");
}

void DestroyFX() {
	_DestroyBuffers();
	rasterState->Release();
	RG_DELETE_CLASS(RGetAllocator(), Buffer, quadbuffer);

	RG_DELETE_CLASS(RGetAllocator(), Shader, lightShader);
	RG_DELETE_CLASS(RGetAllocator(), Shader, aberrationShader);
	RG_DELETE_CLASS(RGetAllocator(), Shader, tonemappingShader);

}

void ResizeFX(ivec2* size) {
	_DestroyBuffers();
	_CreateBuffers(size);
}

void DoPostprocess() {

	ID3D11Buffer* vbuffer = quadbuffer->GetHandle();
	UINT stride = sizeof(Float32) * 2;
	UINT offset = 0;

	ID3D11DeviceContext* ctx = DX11_GetContext();
	ctx->RSSetState(rasterState);

	ID3D11ShaderResourceView* res0 = NULL;
	ID3D11ShaderResourceView* res1 = NULL;

	// Light
	ctx->OMSetRenderTargets(1, &lightpass.rtView, NULL);
	lightShader->Bind();
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	res0 = GetGBufferShaderResource(0);
	res1 = GetLightpassShaderResource();
	ctx->PSSetShaderResources(0, 1, &res0);
	ctx->PSSetShaderResources(1, 1, &res1);
	ctx->Draw(6, 0);

	// Aberration
	ctx->OMSetRenderTargets(1, &aberration.rtView, NULL);
	aberrationShader->Bind();
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	res0 = lightpass.resView;
	ctx->PSSetShaderResources(0, 1, &res0);
	ctx->Draw(6, 0);

	// Tonemapping
	ctx->OMSetRenderTargets(1, &tonemapping.rtView, NULL);
	tonemappingShader->Bind();
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	res0 = aberration.resView;
	ctx->PSSetShaderResources(0, 1, &res0);
	ctx->Draw(6, 0);
}

ID3D11ShaderResourceView* FXGetOuputTexture() {
	return tonemapping.resView;
}