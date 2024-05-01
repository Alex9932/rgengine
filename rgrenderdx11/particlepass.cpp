#include "particlepass.h"
#include "dx11.h"

#include "r3d.h"

#include "gbuffer.h"
#include "shader.h"
#include "rgrenderdx11.h"

#include <filesystem.h>
#include "particleemitters.h"

// Render target
static RenderTarget             rendertarget;

// States
static ID3D11BlendState*        blendState        = NULL;
static ID3D11RasterizerState*   rasterState       = NULL;
static ID3D11DepthStencilState* depthStencilState = NULL;

static ivec2 viewport;

static Shader* shader;

static Buffer* gBuffer;
static Buffer* pBuffer;

static struct GSBuffer {
    mat4 viewproj;
} gsBuffer;

static PSBuffer psBuffer;

ID3D11ShaderResourceView* GetParticleShaderResource() {
    return rendertarget.resView;
}

static void _CreateFramebuffer(ivec2* size) {
    viewport = *size;
    DX11_MakeRenderTarget(&rendertarget, size, DXGI_FORMAT_R16G16B16A16_FLOAT);
}

static void _DestroyFramebuffer() {
    DX11_FreeRenderTarget(&rendertarget);
}

void CreateParticlePass(ivec2* size) {

    PipelineDescription desc = {};
    desc.inputCount   = 0;
    desc.descriptions = NULL;

    char vs[128];
    char ps[128];
    char gs[128];
    Engine::GetPath(vs, 128, RG_PATH_SYSTEM, "shadersdx11/particle.vs");
    Engine::GetPath(ps, 128, RG_PATH_SYSTEM, "shadersdx11/particle.ps");
    Engine::GetPath(gs, 128, RG_PATH_SYSTEM, "shadersdx11/particle.gs");

    shader = RG_NEW_CLASS(RGetAllocator(), Shader)(&desc, vs, ps, gs, false);

    BufferCreateInfo bInfo = {};
    bInfo.access = BUFFER_CPU_WRITE;
    bInfo.usage  = BUFFER_DYNAMIC;
    bInfo.type   = BUFFER_CONSTANT;
    bInfo.length = sizeof(GSBuffer);
    gBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&bInfo);
    bInfo.length = sizeof(PSBuffer);
    pBuffer = RG_NEW_CLASS(RGetAllocator(), Buffer)(&bInfo);

    _CreateFramebuffer(size);

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable                 = false;
    blendDesc.IndependentBlendEnable                = false;
    blendDesc.RenderTarget[0].BlendEnable           = true;
    blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    DX11_GetDevice()->CreateBlendState(&blendDesc, &blendState);

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable                  = true;
    depthStencilDesc.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ZERO; // No depth write (just read only)
    depthStencilDesc.DepthFunc                    = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable                = true;
    depthStencilDesc.StencilReadMask              = 0xFF;
    depthStencilDesc.StencilWriteMask             = 0xFF;
    depthStencilDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;
    DX11_GetDevice()->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);

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
}

void DestroyParticlePass() {
    _DestroyFramebuffer();
    blendState->Release();
    rasterState->Release();
    depthStencilState->Release();
    RG_DELETE_CLASS(RGetAllocator(), Shader, shader);

    RG_DELETE_CLASS(RGetAllocator(), Buffer, gBuffer);
    RG_DELETE_CLASS(RGetAllocator(), Buffer, pBuffer);
}

void ResizeParticlePass(ivec2* size) {
    _DestroyFramebuffer();
    _CreateFramebuffer(size);
}

void RenderParticles() {
    ID3D11DeviceContext* ctx = DX11_GetContext();

    ctx->OMSetRenderTargets(1, &rendertarget.rtView, GetGBufferDepthView());
    ctx->OMSetDepthStencilState(depthStencilState, 1);
    ctx->RSSetState(rasterState);
    vec4 clearColor = {0, 0, 0, 1};
    ctx->ClearRenderTargetView(rendertarget.rtView, clearColor.array);

    float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(blendState, blendFactor, 0xffffffff);

    DX11_SetViewport(viewport.x, viewport.y);

    ctx->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    shader->Bind();

    // Render particles
    //ctx->DrawInstanced(1, (max_particles), 0, 0);

    gsBuffer.viewproj = *GetCameraProjection() * *GetCameraView();
    gBuffer->SetData(0, sizeof(GSBuffer), &gsBuffer);

    for (Uint32 i = 0; i < GetEmitterCount(); i++) {

        GetEmitterInfo(i, &psBuffer);

        pBuffer->SetData(0, sizeof(PSBuffer), &psBuffer);

        ID3D11Buffer* geomBuffer = gBuffer->GetHandle();
        ID3D11Buffer* pixelBuffer = pBuffer->GetHandle();

        // VS
        R3D_AtlasHandle* atlas = GetEmitterAtlasHandle(i);
        R3D_ParticleBuffer* buffer = GetEmitterBufferHandle(i);
        ctx->VSSetShaderResources(0, 1, &buffer->resourceView);

        // GS
        ctx->GSSetConstantBuffers(0, 1, &geomBuffer); // Matrix buffer
        
        // PS
        atlas->texture->Bind(0);        // Texture
        //ctx->PSSetConstantBuffers(0, 1, &pixelBuffer); // Emitter settings

        ctx->DrawInstanced(1, GetEmitterMaxParticle(i), 0, 0);
    }


    ctx->OMSetBlendState(NULL, blendFactor, 0xffffffff);

}