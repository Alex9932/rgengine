#include "postprocess.h"
#include "rgrenderdx11.h"
#include "shader.h"
#include "buffer.h"

#include "gbuffer.h"
#include "lightpass.h"

#include <filesystem.h>
#include <allocator.h>

#include <engine.h>

/*
float redOffset = 0.009;
float greenOffset = 0.006;
float blueOffset = -0.006;
float2 mouseFocusPoint = { 0.509167, 0.598 };
*/

static Buffer* quadbuffer = NULL;
static Float32 quadvertices[] = {
	-1,  1,  1,  1,  1, -1,
	 1, -1, -1, -1, -1,  1
};

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
	pipelineDescription.sampler = &sampler;
	pipelineDescription.inputCount = 1;
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

class FX {
	public:
		FX(ivec2* size, String shader) {
			DX11_MakeRenderTarget(&m_target, size, DXGI_FORMAT_R16G16B16A16_FLOAT);
			m_shader = _MakeShader("base.vs", shader);
		}

		virtual ~FX() {
			DX11_FreeRenderTarget(&m_target);
			RG_DELETE_CLASS(RGetAllocator(), Shader, m_shader);
		}

		void Resize(ivec2* size) {
			DX11_FreeRenderTarget(&m_target);
			DX11_MakeRenderTarget(&m_target, size, DXGI_FORMAT_R16G16B16A16_FLOAT);
		}

		void SetInput(Uint32 idx, ID3D11ShaderResourceView* res) {
			m_input[idx] = res;
		}

		ID3D11ShaderResourceView* GetOutput() {
			return m_target.resView;
		}

		void SetConstants(ID3D11Buffer* b) {
			m_constants = b;
		}

		void Draw() {
			ID3D11DeviceContext* ctx = DX11_GetContext();
			ID3D11Buffer* vbuffer = quadbuffer->GetHandle();
			UINT stride = sizeof(Float32) * 2;
			UINT offset = 0;

			ctx->OMSetRenderTargets(1, &m_target.rtView, NULL);
			m_shader->Bind();

			ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
			ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (m_constants) {
				ctx->PSSetConstantBuffers(0, 1, &m_constants);
			}

			for (Uint32 i = 0; i < 16; i++) {
				ctx->PSSetShaderResources(i, 1, &m_input[i]);
			}
			ctx->Draw(6, 0);
		}

	private:
		RenderTarget m_target = {};
		Shader*      m_shader = NULL;

		ID3D11ShaderResourceView* m_input[16] = {}; // Max 16 input images
		ID3D11Buffer*             m_constants = NULL;


};

static ivec2 viewport    = {};
static ivec2 subViewport = {};

struct ABufferData {
	vec4 offset;
	vec4 focusPoint;
};

struct BBufferData {
	float Directions;
	float Quality;
	float Size;
	int   ScreenSize;
};

static FX* lightpass;

static FX* aberration;
static Buffer* aberrationBuffer;
static ABufferData  aberrationData;

static FX* contrast;

static FX* blurx;
static FX* blury;
static Buffer* blurBuffer;
static BBufferData  blurData;

static FX* mix;

static FX* tonemapping;

//static RenderTarget lightpass;
//static Shader*      lightShader;

//static RenderTarget aberration;
//static Shader*      aberrationShader;
//static Buffer*      aberrationBuffer;
//static ABufferData  aberrationData;

//static RenderTarget contrast;
//static Shader*      contrastShader;

//static RenderTarget tonemapping;
//static Shader*      tonemappingShader;

//static RenderTarget blur;
//static Shader*      blurShader;
//static Buffer*      blurBuffer;
//static BBufferData  blurData;

//static RenderTarget mix;
//static Shader*      mixShader;

static ID3D11RasterizerState* rasterState = NULL;

static void _CreateBuffers(ivec2* size) {
	viewport.x = size->x;
	viewport.y = size->y;
	subViewport.x = size->x;// / 4;
	subViewport.y = size->y;// / 4;

	//DX11_MakeRenderTarget(&lightpass,   &viewport,    DXGI_FORMAT_R16G16B16A16_FLOAT);
	//DX11_MakeRenderTarget(&aberration,  &viewport,    DXGI_FORMAT_R16G16B16A16_FLOAT);
	//DX11_MakeRenderTarget(&tonemapping, &viewport,    DXGI_FORMAT_R16G16B16A16_FLOAT);
	//DX11_MakeRenderTarget(&contrast,    &subViewport, DXGI_FORMAT_R16G16B16A16_FLOAT);
	//DX11_MakeRenderTarget(&blur,        &subViewport, DXGI_FORMAT_R16G16B16A16_FLOAT);
	//DX11_MakeRenderTarget(&mix,         &viewport,    DXGI_FORMAT_R16G16B16A16_FLOAT);
}

static void _DestroyBuffers() {
	//DX11_FreeRenderTarget(&lightpass);
	//DX11_FreeRenderTarget(&aberration);
	//DX11_FreeRenderTarget(&tonemapping);
	//DX11_FreeRenderTarget(&contrast);
	//DX11_FreeRenderTarget(&blur);
	//DX11_FreeRenderTarget(&mix);
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
	vbufferInfo.type        = BUFFER_VERTEX;
	vbufferInfo.access      = BUFFER_GPU_ONLY;
	vbufferInfo.usage       = BUFFER_DEFAULT;
	vbufferInfo.length      = sizeof(quadvertices);
	vbufferInfo.initialData = quadvertices;
	quadbuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);

	BufferCreateInfo abufferInfo = {};
	abufferInfo.type        = BUFFER_CONSTANT;
	abufferInfo.access      = BUFFER_CPU_WRITE;
	abufferInfo.usage       = BUFFER_DYNAMIC;
	abufferInfo.length      = sizeof(ABufferData);
	aberrationBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&abufferInfo);

	abufferInfo.length      = sizeof(BBufferData);
	blurBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&abufferInfo);


	lightpass   = RG_NEW_CLASS(RGetAllocator(), FX)(size, "light.ps");
	aberration  = RG_NEW_CLASS(RGetAllocator(), FX)(size, "aberration.ps");
	tonemapping = RG_NEW_CLASS(RGetAllocator(), FX)(size, "tonemapping.ps");
	contrast    = RG_NEW_CLASS(RGetAllocator(), FX)(size, "contrast.ps");

	blurx = RG_NEW_CLASS(RGetAllocator(), FX)(&subViewport, "blurx.ps");
	blury = RG_NEW_CLASS(RGetAllocator(), FX)(&subViewport, "blury.ps");

	mix = RG_NEW_CLASS(RGetAllocator(), FX)(size, "mix.ps");

	//lightShader       = _MakeShader("base.vs", "light.ps");
	//aberrationShader  = _MakeShader("base.vs", "aberration.ps");
	//tonemappingShader = _MakeShader("base.vs", "tonemapping.ps");
	//contrastShader    = _MakeShader("base.vs", "contrast.ps");
	//blurShader        = _MakeShader("base.vs", "blurx.ps");
	//mixShader         = _MakeShader("base.vs", "mix.ps");
}

void DestroyFX() {
	_DestroyBuffers();
	rasterState->Release();
	RG_DELETE_CLASS(RGetAllocator(), Buffer, quadbuffer);

	RG_DELETE_CLASS(RGetAllocator(), Buffer, aberrationBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, blurBuffer);

	RG_DELETE_CLASS(RGetAllocator(), FX, lightpass);
	RG_DELETE_CLASS(RGetAllocator(), FX, aberration);
	RG_DELETE_CLASS(RGetAllocator(), FX, tonemapping);
	RG_DELETE_CLASS(RGetAllocator(), FX, contrast);

	RG_DELETE_CLASS(RGetAllocator(), FX, blurx);
	RG_DELETE_CLASS(RGetAllocator(), FX, blury);

	RG_DELETE_CLASS(RGetAllocator(), FX, mix);

	//RG_DELETE_CLASS(RGetAllocator(), Shader, lightShader);
	//RG_DELETE_CLASS(RGetAllocator(), Shader, aberrationShader);
	//RG_DELETE_CLASS(RGetAllocator(), Shader, tonemappingShader);
	//RG_DELETE_CLASS(RGetAllocator(), Shader, contrastShader);
	//RG_DELETE_CLASS(RGetAllocator(), Shader, blurShader);
	//RG_DELETE_CLASS(RGetAllocator(), Shader, mixShader);
}

void ResizeFX(ivec2* size) {
	_DestroyBuffers();
	_CreateBuffers(size);

	lightpass->Resize(size);
	aberration->Resize(size);
	tonemapping->Resize(size);
	contrast->Resize(size);
	blurx->Resize(&subViewport);
	blury->Resize(&subViewport);
	mix->Resize(size);
}

void DoPostprocess() {

	ID3D11Buffer* vbuffer = quadbuffer->GetHandle();
	UINT stride = sizeof(Float32) * 2;
	UINT offset = 0;

	ID3D11DeviceContext* ctx = DX11_GetContext();
	ctx->RSSetState(rasterState);

	// Light
	lightpass->SetInput(0, GetGBufferShaderResource(0));
	lightpass->SetInput(1, GetLightpassShaderResource());
	lightpass->Draw();

	// Contrast
	contrast->SetInput(0, lightpass->GetOutput());
	contrast->Draw();

	// Blur
	blurData.Directions = 16.0;
	blurData.Quality = 3.0;
	blurData.Size = 8.0;
	Uint32 screenx = viewport.x & 0x0000FFFF;
	Uint32 screeny = viewport.y & 0x0000FFFF;
	Uint32 shiftx = screenx << 16;
	blurData.ScreenSize = shiftx | screeny;
	blurBuffer->SetData(0, sizeof(BBufferData), &blurData);

	DX11_SetViewport(subViewport.x, subViewport.y);

	blurx->SetInput(0, contrast->GetOutput());
	blurx->SetConstants(blurBuffer->GetHandle());
	blurx->Draw();

	blury->SetInput(0, blurx->GetOutput());
	blury->SetConstants(blurBuffer->GetHandle());
	blury->Draw();

	DX11_SetViewport(viewport.x, viewport.y);

	// Mix
	mix->SetInput(0, blury->GetOutput());
	mix->SetInput(1, lightpass->GetOutput());
	mix->Draw();

	// Tonemapping
	tonemapping->SetInput(0, mix->GetOutput());
	tonemapping->Draw();


#if 0
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
#endif

#if 0
	aberrationData.offset.r = 0.009;
	aberrationData.offset.g = 0.006;
	aberrationData.offset.b = -0.006;
	aberrationData.focusPoint.x = 0.509167;
	aberrationData.focusPoint.y = 0.598;
	aberrationBuffer->SetData(0, sizeof(ABufferData), &aberrationData);
	// Aberration
	ctx->OMSetRenderTargets(1, &aberration.rtView, NULL);
	aberrationShader->Bind();
	buffer_handle = aberrationBuffer->GetHandle();
	ctx->PSSetConstantBuffers(0, 1, &buffer_handle);
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	res0 = lightpass.resView;
	ctx->PSSetShaderResources(0, 1, &res0);
	ctx->Draw(6, 0);
#endif

#if 0
	DX11_SetViewport(subViewport.x, subViewport.y);	

	// Contrast
	ctx->OMSetRenderTargets(1, &contrast.rtView, NULL);
	contrastShader->Bind();
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	res0 = lightpass->GetOutput();
	ctx->PSSetShaderResources(0, 1, &res0);
	ctx->Draw(6, 0);


	blurData.Directions = 16.0;
	blurData.Quality = 3.0;
	blurData.Size = 8.0;

	Uint32 screenx = viewport.x & 0x0000FFFF;
	Uint32 screeny = viewport.y & 0x0000FFFF;
	Uint32 shiftx = screenx << 16;
	blurData.ScreenSize = shiftx | screeny;

	// Blur
	blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	ctx->OMSetRenderTargets(1, &blur.rtView, NULL);
	blurShader->Bind();
	buffer_handle = blurBuffer->GetHandle();
	ctx->PSSetConstantBuffers(0, 1, &buffer_handle);
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//res0 = contrast.resView;
	res0 = lightpass->GetOutput();
	ctx->PSSetShaderResources(0, 1, &res0);
	ctx->Draw(6, 0);

	DX11_SetViewport(viewport.x, viewport.y);


	ctx->OMSetRenderTargets(1, &mix.rtView, NULL);
	mixShader->Bind();
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	res0 = blur.resView;
	res1 = lightpass->GetOutput();
	ctx->PSSetShaderResources(0, 1, &res0);
	ctx->PSSetShaderResources(1, 1, &res1);
	ctx->Draw(6, 0);


	// Tonemapping
	ctx->OMSetRenderTargets(1, &tonemapping.rtView, NULL);
	tonemappingShader->Bind();
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	res0 = mix.resView;
	ctx->PSSetShaderResources(0, 1, &res0);
	ctx->Draw(6, 0);
#endif
}

ID3D11ShaderResourceView* FXGetOuputTexture() {
	return tonemapping->GetOutput();
}