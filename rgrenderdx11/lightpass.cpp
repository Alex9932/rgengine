#include "lightpass.h"

#include <rgmath.h>
#include <render.h>
#include <lightsystem.h>
#include <filesystem.h>

#include "shader.h"
#include "buffer.h"
#include "rgrenderdx11.h"

#include "r3d.h"
#include "gbuffer.h"
#include "shadowbuffer.h"

Float32 sphere_vertices[] = {
    0.000000, -1.000000,  0.000000,
    0.723607, -0.447220,  0.525725,
    -0.276388, -0.447220,  0.850649,
    -0.894426, -0.447216,  0.000000,
    -0.276388, -0.447220, -0.850649,
    0.723607, -0.447220, -0.525725,
    0.276388,  0.447220,  0.850649,
    -0.723607,  0.447220,  0.525725,
    -0.723607,  0.447220, -0.525725,
    0.276388,  0.447220, -0.850649,
    0.894426,  0.447216,  0.000000,
    0.000000,  1.000000,  0.000000,
    -0.162456, -0.850654,  0.499995,
    0.425323, -0.850654,  0.309011,
    0.262869, -0.525738,  0.809012,
    0.850648, -0.525736,  0.000000,
    0.425323, -0.850654, -0.309011,
    -0.525730, -0.850652,  0.000000,
    -0.688189, -0.525736,  0.499997,
    -0.162456, -0.850654, -0.499995,
    -0.688189, -0.525736, -0.499997,
    0.262869, -0.525738, -0.809012,
    0.951058,  0.000000,  0.309013,
    0.951058,  0.000000, -0.309013,
    0.000000,  0.000000,  1.000000,
    0.587786,  0.000000,  0.809017,
    -0.951058,  0.000000,  0.309013,
    -0.587786,  0.000000,  0.809017,
    -0.587786,  0.000000, -0.809017,
    -0.951058,  0.000000, -0.309013,
    0.587786,  0.000000, -0.809017,
    0.000000,  0.000000, -1.000000,
    0.688189,  0.525736,  0.499997,
    -0.262869,  0.525738,  0.809012,
    -0.850648,  0.525736,  0.000000,
    -0.262869,  0.525738, -0.809012,
    0.688189,  0.525736, -0.499997,
    0.162456,  0.850654,  0.499995,
    0.525730,  0.850652,  0.000000,
    -0.425323,  0.850654,  0.309011,
    -0.425323,  0.850654, -0.309011,
    0.162456,  0.850654, -0.499995
};
Uint16 sphere_indices[] = {
    0, 13, 12,
    1, 13, 15,
    0, 12, 17,
    0, 17, 19,
    0, 19, 16,
    1, 15, 22,
    2, 14, 24,
    3, 18, 26,
    4, 20, 28,
    5, 21, 30,
    1, 22, 25,
    2, 24, 27,
    3, 26, 29,
    4, 28, 31,
    5, 30, 23,
    6, 32, 37,
    7, 33, 39,
    8, 34, 40,
    9, 35, 41,
    10, 36, 38,
    38, 41, 11,
    38, 36, 41,
    36, 9, 41,
    41, 40, 11,
    41, 35, 40,
    35, 8, 40,
    40, 39, 11,
    40, 34, 39,
    34, 7, 39,
    39, 37, 11,
    39, 33, 37,
    33, 6, 37,
    37, 38, 11,
    37, 32, 38,
    32, 10, 38,
    23, 36, 10,
    23, 30, 36,
    30, 9, 36,
    31, 35, 9,
    31, 28, 35,
    28, 8, 35,
    29, 34, 8,
    29, 26, 34,
    26, 7, 34,
    27, 33, 7,
    27, 24, 33,
    24, 6, 33,
    25, 32, 6,
    25, 22, 32,
    22, 10, 32,
    30, 31, 9,
    30, 21, 31,
    21, 4, 31,
    28, 29, 8,
    28, 20, 29,
    20, 3, 29,
    26, 27, 7,
    26, 18, 27,
    18, 2, 27,
    24, 25, 6,
    24, 14, 25,
    14, 1, 25,
    22, 23, 10,
    22, 15, 23,
    15, 5, 23,
    16, 21, 5,
    16, 19, 21,
    19, 4, 21,
    19, 20, 4,
    19, 17, 20,
    17, 3, 20,
    17, 18, 3,
    17, 12, 18,
    12, 2, 18,
    15, 16, 5,
    15, 13, 16,
    13, 0, 16,
    12, 14, 2,
    12, 13, 14,
    13, 1, 14
};

Float32 quad_vertices[] = {
    -1, -1, 0,
    -1,  1, 0,
     1,  1, 0,
     1, -1, 0,
};
Uint16 quad_indices[] = {
    0, 1, 2, 2, 3, 0
};

struct ShaderConstants {
	mat4 viewproj;
	mat4 model;
};

struct ShaderLight {
    vec3    viewPos;
    float   offset;
	vec3    color;
	Float32 intensity;
	vec3    pos;
	Uint32  screenSize;
    mat4    lightMatrix;
};

static Shader*         globalshader = NULL;
static Shader*         pointshader  = NULL;

static Buffer*         cBuffer      = NULL;
static Buffer*         lBuffer      = NULL;
static ShaderConstants constants    = {};
static ShaderLight     light        = {};

static GlobalLight     globallight  = {};

static ivec2           screenSize   = {0, 0};

static Buffer*         vBuffer      = NULL;
static Buffer*         iBuffer      = NULL;

static Buffer*         vBuffer_quad = NULL;
static Buffer*         iBuffer_quad = NULL;

static ID3D11Texture2D*          outputBuffer  = NULL;
static ID3D11ShaderResourceView* outputResView = NULL;
static ID3D11RenderTargetView*   outputView    = NULL;

static ID3D11RasterizerState*    rasterState   = NULL;

static ID3D11BlendState*         blendState    = NULL;

static mat4 lightmatrix = {};

static void _CreateBuffers(ivec2* size) {
	Uint32 screenx = size->x & 0x0000FFFF;
	Uint32 screeny = size->y & 0x0000FFFF;
	Uint32 shiftx = screenx << 16;
	light.screenSize = shiftx | screeny;

	DX11_MakeTexture(&outputBuffer, &outputResView, size, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
	DX11_GetDevice()->CreateRenderTargetView(outputBuffer, NULL, &outputView);

}

static void _DestroyBuffers() {
	outputBuffer->Release();
	outputResView->Release();
	outputView->Release();
}

void CreateLightpass(ivec2* size) {

	InputDescription inputDescription = {};
	inputDescription.name      = "POSITION";
	inputDescription.inputSlot = 0;
	inputDescription.format    = INPUT_R32G32B32_FLOAT;
	PipelineDescription description = {};
	description.inputCount   = 1;
	description.descriptions = &inputDescription;

    char lp_vs[128];
    char lp_ps[128];
    Engine::GetPath(lp_vs, 128, RG_PATH_SYSTEM, "shadersdx11/accum_common.vs");
    Engine::GetPath(lp_ps, 128, RG_PATH_SYSTEM, "shadersdx11/accum_global.ps");
    globalshader = RG_NEW_CLASS(RGetAllocator(), Shader)(&description, lp_vs, lp_ps, false);

    Engine::GetPath(lp_ps, 128, RG_PATH_SYSTEM, "shadersdx11/accum_point.ps");
	pointshader = RG_NEW_CLASS(RGetAllocator(), Shader)(&description, lp_vs, lp_ps, false);

	BufferCreateInfo bInfo = {};
	bInfo.access = BUFFER_CPU_WRITE;
	bInfo.usage  = BUFFER_DYNAMIC;
	bInfo.type   = BUFFER_CONSTANT;
	bInfo.length = sizeof(ShaderConstants);
	cBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&bInfo);
	bInfo.length = sizeof(ShaderLight);
	lBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&bInfo);

    BufferCreateInfo vbufferInfo;
    BufferCreateInfo ibufferInfo;

    // Sphere for point light
    vbufferInfo = {};
    vbufferInfo.type   = BUFFER_VERTEX;
    vbufferInfo.access = BUFFER_GPU_ONLY;
    vbufferInfo.usage  = BUFFER_DEFAULT;
    vbufferInfo.length = sizeof(sphere_vertices);
    vBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);
    vBuffer->SetData(0, vbufferInfo.length, sphere_vertices);

    ibufferInfo = {};
    ibufferInfo.type   = BUFFER_INDEX;
    ibufferInfo.access = BUFFER_GPU_ONLY;
    ibufferInfo.usage  = BUFFER_DEFAULT;
    ibufferInfo.length = sizeof(sphere_indices);
    iBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&ibufferInfo);
    iBuffer->SetData(0, ibufferInfo.length, sphere_indices);

    // Quad for global light
    vbufferInfo = {};
    vbufferInfo.type = BUFFER_VERTEX;
    vbufferInfo.access = BUFFER_GPU_ONLY;
    vbufferInfo.usage = BUFFER_DEFAULT;
    vbufferInfo.length = sizeof(quad_vertices);
    vBuffer_quad = RG_NEW_CLASS(RGetAllocator(), Buffer)(&vbufferInfo);
    vBuffer_quad->SetData(0, vbufferInfo.length, quad_vertices);

    ibufferInfo = {};
    ibufferInfo.type = BUFFER_INDEX;
    ibufferInfo.access = BUFFER_GPU_ONLY;
    ibufferInfo.usage = BUFFER_DEFAULT;
    ibufferInfo.length = sizeof(quad_indices);
    iBuffer_quad = RG_NEW_CLASS(RGetAllocator(), Buffer)(&ibufferInfo);
    iBuffer_quad->SetData(0, ibufferInfo.length, quad_indices);

    

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode              = D3D11_CULL_BACK;
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
    blendDesc.AlphaToCoverageEnable  = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable    = true;
    blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    DX11_GetDevice()->CreateBlendState(&blendDesc, &blendState);

	_CreateBuffers(size);


    mat4 lproj;
    mat4 lview;

    mat4_ortho(&lproj, -20, 20, -20, 20, -50, 50);
    mat4_view(&lview, { -15,20,0 }, { 0.777f, 1.6, 0 });

    lightmatrix = lproj * lview;

    rgLogInfo(RG_LOG_RENDER, "Matrix:\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f",
        lproj.m00, lproj.m01, lproj.m02, lproj.m03,
        lproj.m10, lproj.m11, lproj.m12, lproj.m13,
        lproj.m20, lproj.m21, lproj.m22, lproj.m23,
        lproj.m30, lproj.m31, lproj.m32, lproj.m33
    );

    rgLogInfo(RG_LOG_RENDER, "Matrix:\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f",
        lview.m00, lview.m01, lview.m02, lview.m03,
        lview.m10, lview.m11, lview.m12, lview.m13,
        lview.m20, lview.m21, lview.m22, lview.m23,
        lview.m30, lview.m31, lview.m32, lview.m33
    );

    rgLogInfo(RG_LOG_RENDER, "Matrix:\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f",
        lightmatrix.m00, lightmatrix.m01, lightmatrix.m02, lightmatrix.m03,
        lightmatrix.m10, lightmatrix.m11, lightmatrix.m12, lightmatrix.m13,
        lightmatrix.m20, lightmatrix.m21, lightmatrix.m22, lightmatrix.m23,
        lightmatrix.m30, lightmatrix.m31, lightmatrix.m32, lightmatrix.m33
    );

}

void DestroyLightpass() {
	rasterState->Release();
    blendState->Release();
	_DestroyBuffers();
    RG_DELETE_CLASS(RGetAllocator(), Shader, globalshader);
    RG_DELETE_CLASS(RGetAllocator(), Shader, pointshader);
    RG_DELETE_CLASS(RGetAllocator(), Buffer, cBuffer);
    RG_DELETE_CLASS(RGetAllocator(), Buffer, lBuffer);
    RG_DELETE_CLASS(RGetAllocator(), Buffer, vBuffer);
    RG_DELETE_CLASS(RGetAllocator(), Buffer, iBuffer);
    RG_DELETE_CLASS(RGetAllocator(), Buffer, vBuffer_quad);
    RG_DELETE_CLASS(RGetAllocator(), Buffer, iBuffer_quad);
}

void ResizeLightpass(ivec2* size) {
	_DestroyBuffers();
	_CreateBuffers(size);
}

void DoLightpass() {

    // TMP
    globallight.color     = { 1, 0.8f, 0.7f };
    globallight.intensity = 10;
    globallight.direction = { 1, -1, 1 };
    globallight.ambient   = 0.4f;

    float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	ID3D11DeviceContext* ctx = DX11_GetContext();
	ctx->OMSetRenderTargets(1, &outputView, NULL);
    ctx->OMSetBlendState(blendState, blendFactor, 0xffffffff);
	ctx->RSSetState(rasterState);

	Float32 clearColor[] = { 0, 0, 0, 1 };
	ctx->ClearRenderTargetView(outputView, clearColor);

    ID3D11ShaderResourceView* resourceViews[4]; // 4-th shadowmap
    resourceViews[0] = GetGBufferShaderResource(0);
    resourceViews[1] = GetGBufferShaderResource(1);
    resourceViews[2] = GetGBufferShaderResource(2);

    light.viewPos = *GetCameraPosition();

    // Bind shader & input resources
	pointshader->Bind();
    DX11_GetContext()->PSSetShaderResources(0, 3, resourceViews);

    // Bind buffers
	UINT stride = sizeof(Float32) * 3;
	UINT offset = 0;
	ID3D11Buffer* vbuffer = vBuffer->GetHandle();
	ctx->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
	ctx->IASetIndexBuffer(iBuffer->GetHandle(), DXGI_FORMAT_R16_UINT, 0);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11Buffer* constBuffer = cBuffer->GetHandle();
	ID3D11Buffer* lightBuffer = lBuffer->GetHandle();
	ctx->VSSetConstantBuffers(0, 1, &constBuffer);
	ctx->PSSetConstantBuffers(0, 1, &lightBuffer);

	// Draw point lights
	Engine::LightSystem* system = Engine::Render::GetLightSystem();

	constants.viewproj = *GetCameraProjection() * *GetCameraView();
	constants.model    = MAT4_IDENTITY();

	Uint32 pCount = system->GetPointLightCount();
	for (Uint32 i = 0; i < pCount; i++) {

		Engine::PointLight* l = system->GetPointLight(i);

		light.color     = l->GetColor();
		light.intensity = l->GetIntensity();
		light.pos       = l->GetPosition();

        Float32 s = light.intensity * 10;
        mat4_model(&constants.model, l->GetPosition(), { 0, 0, 0 }, {s, s, s});

		cBuffer->SetData(0, sizeof(ShaderConstants), &constants);
		lBuffer->SetData(0, sizeof(ShaderLight), &light);

        DX11_GetContext()->DrawIndexed(sizeof(sphere_indices) / sizeof(Uint16), 0, 0);

	}

    // Bind shader & input resources
    globalshader->Bind();
    resourceViews[3] = GetShadowDepth();
    DX11_GetContext()->PSSetShaderResources(0, 4, resourceViews);

    // Bind buffers
    stride = sizeof(Float32) * 3;
    offset = 0;
    ID3D11Buffer* vbuffer_quad = vBuffer_quad->GetHandle();
    ctx->IASetVertexBuffers(0, 1, &vbuffer_quad, &stride, &offset);
    ctx->IASetIndexBuffer(iBuffer_quad->GetHandle(), DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    constBuffer = cBuffer->GetHandle();
    lightBuffer = lBuffer->GetHandle();
    ctx->VSSetConstantBuffers(0, 1, &constBuffer);
    ctx->PSSetConstantBuffers(0, 1, &lightBuffer);

    // Draw global light
    constants.viewproj = MAT4_IDENTITY();
    constants.model    = MAT4_IDENTITY();

    light.color       = globallight.color;
    light.intensity   = globallight.intensity;
    light.pos         = globallight.direction; // NO POSITION JUST DIRECTION!!!
    light.offset      = globallight.ambient;
    light.lightMatrix = lightmatrix;

    cBuffer->SetData(0, sizeof(ShaderConstants), &constants);
    lBuffer->SetData(0, sizeof(ShaderLight), &light);

    DX11_GetContext()->DrawIndexed(sizeof(quad_indices) / sizeof(Uint16), 0, 0);

    ctx->OMSetBlendState(NULL, blendFactor, 0xffffffff);
}

void SetGlobalLight(GlobalLight* light) {
    globallight = *light;
}

ID3D11ShaderResourceView* GetLightpassShaderResource() {
	return outputResView;
}

mat4* GetLightMatrix() {
    return &lightmatrix;
}