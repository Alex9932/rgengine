#include "postprocess.h"
#include "rgrenderdx11.h"
#include "shader.h"
#include "buffer.h"

#include "gbuffer.h"
#include "shadowbuffer.h"
#include "lightpass.h"

#include "r3d.h"

#include <filesystem.h>
#include <allocator.h>

#include <rgmatrix.h>

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
			m_shader   = _MakeShader("base.vs", shader);
			m_viewport = *size;
		}

		virtual ~FX() {
			DX11_FreeRenderTarget(&m_target);
			RG_DELETE_CLASS(RGetAllocator(), Shader, m_shader);
		}

		void Resize(ivec2* size) {
			DX11_FreeRenderTarget(&m_target);
			DX11_MakeRenderTarget(&m_target, size, DXGI_FORMAT_R16G16B16A16_FLOAT);
			m_viewport = *size;
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

		ivec2* GetSize() { return &m_viewport; }

		void Draw() {
			DX11_SetViewport(m_viewport.x, m_viewport.y);

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
		RenderTarget m_target   = {};
		Shader*      m_shader   = NULL;
		ivec2        m_viewport = {};

		ID3D11ShaderResourceView* m_input[16] = {}; // Max 16 input images
		ID3D11Buffer*             m_constants = NULL;


};

static ivec2 viewport    = {};

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

struct SSRBufferData {
	mat4 proj;
	mat4 view;
	mat4 viewProj;
	mat4 invProj;
	mat4 invView;
	vec4 camera_position;
	int  screen_size;
};

struct SSAOBufferData {
	mat4 proj;
};

struct GodRaysBufferData {
	mat4 viewproj;
	vec3 lightpos;
	Float32 offset;
};

static FX* lightpass;

static FX*           ssr;
static Buffer*       ssrBuffer;
static SSRBufferData ssrData;


static FX* ssr_blurx1;
static FX* ssr_blury1;

static FX*         aberration;
static Buffer*     aberrationBuffer;
static ABufferData aberrationData;

// Bloom
static FX* contrast;

static FX* blurx1;
static FX* blury1;
static FX* blurx2;
static FX* blury2;
static FX* blurx3;
static FX* blury3;

static Buffer*     blurBuffer;
static BBufferData blurData;

// SSAO
static FX* ssao;
static Buffer* ssaoBuffer;
static SSAOBufferData ssaoData;

// Godrays
static FX* godrays;
static Buffer* godraysBuffer;
static GodRaysBufferData godraysData;

static FX* mix;
static Buffer* mixBuffer;
static struct MixBufferData {
	vec4 camera_position;
	int  screen_size;
} mix_data;

static FX* tonemapping;

static ID3D11RasterizerState* rasterState = NULL;


static void LoadFX() {

	ivec2* size = &viewport;

	lightpass = RG_NEW_CLASS(RGetAllocator(), FX)(size, "light.ps");

	ivec2 ssr_size;
	ssr_size.x = size->x / 4;
	ssr_size.y = size->y / 4;

	ssr = RG_NEW_CLASS(RGetAllocator(), FX)(&ssr_size, "reflection.ps");

	ssr_blurx1 = RG_NEW_CLASS(RGetAllocator(), FX)(size, "blurx.ps");
	ssr_blury1 = RG_NEW_CLASS(RGetAllocator(), FX)(size, "blury.ps");

	aberration = RG_NEW_CLASS(RGetAllocator(), FX)(size, "aberration.ps");
	tonemapping = RG_NEW_CLASS(RGetAllocator(), FX)(size, "tonemapping.ps");
	contrast = RG_NEW_CLASS(RGetAllocator(), FX)(size, "contrast.ps");

	ivec2 subSize = *size;
	subSize.x /= 4;
	subSize.y /= 4;
	blurx1 = RG_NEW_CLASS(RGetAllocator(), FX)(&subSize, "blurx.ps");
	blury1 = RG_NEW_CLASS(RGetAllocator(), FX)(&subSize, "blury.ps");

	subSize.x /= 4;
	subSize.y /= 4;
	blurx2 = RG_NEW_CLASS(RGetAllocator(), FX)(&subSize, "blurx.ps");
	blury2 = RG_NEW_CLASS(RGetAllocator(), FX)(&subSize, "blury.ps");

	subSize.x /= 4;
	subSize.y /= 4;
	blurx3 = RG_NEW_CLASS(RGetAllocator(), FX)(&subSize, "blurx.ps");
	blury3 = RG_NEW_CLASS(RGetAllocator(), FX)(&subSize, "blury.ps");

	mix = RG_NEW_CLASS(RGetAllocator(), FX)(size, "mix.ps");

	ssao = RG_NEW_CLASS(RGetAllocator(), FX)(size, "ssao.ps");

	godrays = RG_NEW_CLASS(RGetAllocator(), FX)(size, "godrays.ps");
}

static void FreeFX() {
	RG_DELETE_CLASS(RGetAllocator(), FX, lightpass);

	RG_DELETE_CLASS(RGetAllocator(), FX, ssr);
	RG_DELETE_CLASS(RGetAllocator(), FX, ssr_blurx1);
	RG_DELETE_CLASS(RGetAllocator(), FX, ssr_blury1);

	RG_DELETE_CLASS(RGetAllocator(), FX, aberration);
	RG_DELETE_CLASS(RGetAllocator(), FX, tonemapping);
	RG_DELETE_CLASS(RGetAllocator(), FX, contrast);

	RG_DELETE_CLASS(RGetAllocator(), FX, blurx1);
	RG_DELETE_CLASS(RGetAllocator(), FX, blury1);
	RG_DELETE_CLASS(RGetAllocator(), FX, blurx2);
	RG_DELETE_CLASS(RGetAllocator(), FX, blury2);
	RG_DELETE_CLASS(RGetAllocator(), FX, blurx3);
	RG_DELETE_CLASS(RGetAllocator(), FX, blury3);

	RG_DELETE_CLASS(RGetAllocator(), FX, mix);
	RG_DELETE_CLASS(RGetAllocator(), FX, ssao);
	RG_DELETE_CLASS(RGetAllocator(), FX, godrays);
}

void CreateFX(ivec2* size) {
	viewport.x = size->x;
	viewport.y = size->y;

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

	abufferInfo.length      = sizeof(SSRBufferData);
	ssrBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&abufferInfo);

	abufferInfo.length = sizeof(SSAOBufferData);
	ssaoBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&abufferInfo);

	abufferInfo.length = sizeof(GodRaysBufferData);
	godraysBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&abufferInfo);

	abufferInfo.length = sizeof(MixBufferData);
	mixBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&abufferInfo);

	LoadFX();
}

void DestroyFX() {
	rasterState->Release();
	RG_DELETE_CLASS(RGetAllocator(), Buffer, quadbuffer);

	RG_DELETE_CLASS(RGetAllocator(), Buffer, aberrationBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, blurBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, ssrBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, ssaoBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, godraysBuffer);
	RG_DELETE_CLASS(RGetAllocator(), Buffer, mixBuffer);

	FreeFX();
}

void ReloadShadersFX() {
	FreeFX();
	LoadFX();
}

void ResizeFX(ivec2* size) {
	viewport.x = size->x;
	viewport.y = size->y;

	lightpass->Resize(size);


	ivec2 ssr_size;
	ssr_size.x = size->x / 4;
	ssr_size.y = size->y / 4;

	ssr->Resize(&ssr_size);

	ssr_blurx1->Resize(size);
	ssr_blury1->Resize(size);

	//ssr->Resize(size);

	aberration->Resize(size);
	tonemapping->Resize(size);
	contrast->Resize(size);

	ivec2 subSize = *size;
	subSize.x /= 4;
	subSize.y /= 4;
	blurx1->Resize(&subSize);
	blury1->Resize(&subSize);

	subSize.x /= 4;
	subSize.y /= 4;
	blurx2->Resize(&subSize);
	blury2->Resize(&subSize);

	subSize.x /= 4;
	subSize.y /= 4;
	blurx3->Resize(&subSize);
	blury3->Resize(&subSize);

	ssao->Resize(size);
	godrays->Resize(size);

	mix->Resize(size);
}

static inline Uint32 PackVector(ivec2* vec) {
	Uint32 x = vec->x & 0x0000FFFF;
	Uint32 y = vec->y & 0x0000FFFF;
	Uint32 shiftx = x << 16;
	return shiftx | y;
}

static ID3D11ShaderResourceView* DoBloom(ID3D11ShaderResourceView* entry) {

	blurData.Directions = 16.0;
	blurData.Quality = 3.0;
	blurData.Size = 8.0;

	// Apply threshold

	contrast->SetInput(0, entry);
	contrast->Draw();

	// Downscale blur

	blurData.ScreenSize = PackVector(blurx1->GetSize());
	blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	blurx1->SetInput(0, contrast->GetOutput());
	blurx1->SetConstants(blurBuffer->GetHandle());
	blurx1->Draw();

	//blurData.ScreenSize = PackVector(blurx1->GetSize());
	//blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	blury1->SetInput(0, blurx1->GetOutput());
	blury1->SetConstants(blurBuffer->GetHandle());
	blury1->Draw();

	blurData.ScreenSize = PackVector(blurx2->GetSize());
	blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	blurx2->SetInput(0, blury1->GetOutput());
	blurx2->SetConstants(blurBuffer->GetHandle());
	blurx2->Draw();

	//blurData.ScreenSize = PackVector(blurx2->GetSize());
	//blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	blury2->SetInput(0, blurx2->GetOutput());
	blury2->SetConstants(blurBuffer->GetHandle());
	blury2->Draw();

	blurData.ScreenSize = PackVector(blurx3->GetSize());
	blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	blurx3->SetInput(0, blury2->GetOutput());
	blurx3->SetConstants(blurBuffer->GetHandle());
	blurx3->Draw();

	//blurData.ScreenSize = PackVector(blurx3->GetSize());
	//blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	blury3->SetInput(0, blurx3->GetOutput());
	blury3->SetConstants(blurBuffer->GetHandle());
	blury3->Draw();

	// Upscale blur

	blurData.ScreenSize = PackVector(blurx2->GetSize());
	blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	blurx2->SetInput(0, blury3->GetOutput());
	blurx2->SetConstants(blurBuffer->GetHandle());
	blurx2->Draw();

	blury2->SetInput(0, blurx2->GetOutput());
	blury2->SetConstants(blurBuffer->GetHandle());
	blury2->Draw();

	blurData.ScreenSize = PackVector(blurx1->GetSize());
	blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	blurx1->SetInput(0, blury2->GetOutput());
	blurx1->SetConstants(blurBuffer->GetHandle());
	blurx1->Draw();

	blury1->SetInput(0, blurx1->GetOutput());
	blury1->SetConstants(blurBuffer->GetHandle());
	blury1->Draw();

	return blury1->GetOutput();
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

	ID3D11ShaderResourceView* lightpass_output = GetLightpassShaderResource();

	// SSR

	ssrData.proj = *GetCameraProjection();
	ssrData.view = *GetCameraView();
	ssrData.viewProj = ssrData.proj * ssrData.view;

	mat4_inverse(&ssrData.invProj, ssrData.proj);
	mat4_inverse(&ssrData.invView, ssrData.view);
	ssrData.camera_position.xyz = *GetCameraPosition();
	ssrData.camera_position.w = 1;

	Uint32 screenx = (viewport.x / 4) & 0x0000FFFF;
	Uint32 screeny = (viewport.y / 4) & 0x0000FFFF;
	Uint32 shiftx = screenx << 16;
	ssrData.screen_size = shiftx | screeny;

	ssrBuffer->SetData(0, sizeof(SSRBufferData), &ssrData);
	ssr->SetInput(0, GetGBufferShaderResource(0)); // Albedo
	ssr->SetInput(1, GetGBufferShaderResource(1)); // Normal
	ssr->SetInput(2, GetGBufferShaderResource(2)); // Wpos
	ssr->SetInput(3, lightpass_output);
	ssr->SetInput(4, GetGBufferDepth());
	ssr->SetConstants(ssrBuffer->GetHandle());
	ssr->Draw();

	// Blur ssr


	blurData.ScreenSize = PackVector(blurx1->GetSize());
	blurBuffer->SetData(0, sizeof(BBufferData), &blurData);
	ssr_blurx1->SetInput(0, ssr->GetOutput());
	ssr_blurx1->SetConstants(blurBuffer->GetHandle());
	ssr_blurx1->Draw();

	ssr_blury1->SetInput(0, ssr_blurx1->GetOutput());
	ssr_blury1->SetConstants(blurBuffer->GetHandle());
	ssr_blury1->Draw();

	//ssr_blury1->GetOutput();


	// SSAO

	ssaoData.proj = *GetCameraProjection();

	ssaoBuffer->SetData(0, sizeof(SSAOBufferData), &ssaoData);
	ssao->SetInput(0, GetGBufferShaderResource(1)); // Normal
	ssao->SetInput(1, GetGBufferShaderResource(2)); // Wpos
	ssao->SetConstants(ssaoBuffer->GetHandle());
	ssao->Draw();

	// Godrays

	godraysData.viewproj = *GetCameraProjection() * *GetCameraView();
	godraysData.lightpos = *GetSunPosition() * 100;// { -7, 100, 2 };

	godraysBuffer->SetData(0, sizeof(GodRaysBufferData), &godraysData);

	//godrays->SetInput(0, GetShadowDepth());  // Light depth-map
	//godrays->SetInput(1, GetGBufferDepth()); // G-Buffer depth-map

	godrays->SetInput(0, GetGBufferShaderResource(0)); // Skybox color
	//godrays->SetInput(1, GetGBufferShaderResource(2)); // Alpha-channel used
	godrays->SetInput(1, GetGBufferDepth()); // Depth buffer

	godrays->SetConstants(godraysBuffer->GetHandle());
	godrays->Draw();

	// Bloom
	ID3D11ShaderResourceView* bloomResult = DoBloom(lightpass_output);
	//ID3D11ShaderResourceView* bloomResult = DoBloom(ssr->GetOutput());

	mix_data.camera_position.xyz = *GetCameraPosition();
	mix_data.camera_position.w = 1;

	screenx = (viewport.x) & 0x0000FFFF;
	screeny = (viewport.y) & 0x0000FFFF;
	shiftx = screenx << 16;
	mix_data.screen_size = shiftx | screeny;
	mixBuffer->SetData(0, sizeof(MixBufferData), &mix_data);

	// Mix
	mix->SetConstants(mixBuffer->GetHandle());

	mix->SetInput(0, bloomResult);
	mix->SetInput(1, lightpass_output);

	mix->SetInput(2, ssr_blury1->GetOutput());     // Reflections
	mix->SetInput(3, GetGBufferShaderResource(1)); // Normal
	mix->SetInput(4, GetGBufferShaderResource(2)); // Wpos

	mix->SetInput(5, godrays->GetOutput());

	mix->Draw();

	// Tonemapping
	tonemapping->SetInput(0, mix->GetOutput());
	tonemapping->Draw();

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
}

static Uint32 viewidx = 0;

void SetViewIDX(Uint32 idx) {
	if (idx > 7) { return; }
	viewidx = idx;
}

ID3D11ShaderResourceView* FXGetOuputTexture() {

	ID3D11ShaderResourceView* views[8];
	views[0] = mix->GetOutput();
	views[1] = godrays->GetOutput();
	//views[2] = ssao->GetOutput();
	views[2] = ssr_blury1->GetOutput();
	views[3] = lightpass->GetOutput();
	views[4] = GetGBufferShaderResource(0);
	views[5] = GetGBufferShaderResource(1);
	views[6] = GetGBufferShaderResource(2);
	views[7] = GetGBufferDepth();

	return views[viewidx];

	//return tonemapping->GetOutput();
	// //return mix->GetOutput();
	//return GetLightpassShaderResource();
	//return ssr->GetOutput();
	//return ssao->GetOutput();
}