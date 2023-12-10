#include "gbuffer.h"
#include "dx11.h"

// Render targets
static RenderTarget             rendertargets[3];

// Depth buffer
static ID3D11Texture2D*          depthBuffer      = NULL;
static ID3D11DepthStencilView*   depthStencilView = NULL;
static ID3D11ShaderResourceView* depthResView     = NULL;

// States
static ID3D11RasterizerState*   rasterState       = NULL;
static ID3D11DepthStencilState* depthStencilState = NULL;

static ivec2 viewport;

ID3D11ShaderResourceView* GetGBufferShaderResource(Uint32 idx) {
    return rendertargets[idx].resView;
}

ID3D11ShaderResourceView* GetGBufferDepth() {
    return depthResView;
}

static void _CreateFramebuffer(ivec2* size) {

    viewport = *size;

    DX11_MakeRenderTarget(&rendertargets[0], size, DXGI_FORMAT_R16G16B16A16_FLOAT);
    DX11_MakeRenderTarget(&rendertargets[1], size, DXGI_FORMAT_R16G16B16A16_FLOAT);
    DX11_MakeRenderTarget(&rendertargets[2], size, DXGI_FORMAT_R16G16B16A16_FLOAT);

    DX11_MakeTexture(&depthBuffer, NULL, size, DXGI_FORMAT_R32_TYPELESS, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;
    HRESULT r = 0;
    r = DX11_GetDevice()->CreateShaderResourceView(depthBuffer, &srvDesc, &depthResView);

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;// DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;
    DX11_GetDevice()->CreateDepthStencilView(depthBuffer, &depthStencilViewDesc, &depthStencilView);

}

static void _DestroyFramebuffer() {

    DX11_FreeRenderTarget(&rendertargets[0]);
    DX11_FreeRenderTarget(&rendertargets[1]);
    DX11_FreeRenderTarget(&rendertargets[2]);

    depthStencilView->Release();
    depthBuffer->Release();
    depthResView->Release();
}

void CreateGBuffer(ivec2* size) {

    _CreateFramebuffer(size);

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    DX11_GetDevice()->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.AntialiasedLineEnable = false;
    //rasterDesc.CullMode = D3D11_CULL_FRONT;
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

void DestroyGBuffer() {
    _DestroyFramebuffer();

    rasterState->Release();
    depthStencilState->Release();
}

void ResizeGbuffer(ivec2* size) {
    _DestroyFramebuffer();
    _CreateFramebuffer(size);
}

void BindGBuffer() {

    ID3D11DeviceContext* ctx = DX11_GetContext();

    ID3D11RenderTargetView* buffers[] = {
        rendertargets[0].rtView,
        rendertargets[1].rtView,
        rendertargets[2].rtView
    };
    ctx->OMSetRenderTargets(3, buffers, depthStencilView);
    ctx->OMSetDepthStencilState(depthStencilState, 1);
    ctx->RSSetState(rasterState);

    //ctx->ClearRenderTargetView(dx_backbuffer, clearColor);
    ctx->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    DX11_SetViewport(viewport.x, viewport.y);

}