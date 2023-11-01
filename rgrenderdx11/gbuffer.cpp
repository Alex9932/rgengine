#include "gbuffer.h"
#include "dx11.h"

static ID3D11Texture2D* albedoBuffer;
static ID3D11Texture2D* normalBuffer;
static ID3D11Texture2D* wposBuffer;
static ID3D11Texture2D* depthBuffer;

static ID3D11ShaderResourceView* resView[3];

static ID3D11RenderTargetView* albedoView;
static ID3D11RenderTargetView* normalView;
static ID3D11RenderTargetView* wposView;
static ID3D11DepthStencilView* depthStencilView;

static ID3D11DepthStencilState* depthStencilState;

static ID3D11RasterizerState* rasterState;

ID3D11ShaderResourceView* GetGBufferShaderResource(Uint32 idx) {
    return resView[idx];
}

static void _MakeTexture(ID3D11Texture2D** buffer, ID3D11ShaderResourceView** resView, ivec2* size, DXGI_FORMAT format, UINT flags) {

    D3D11_TEXTURE2D_DESC tDesc = {};
    tDesc.Width = size->x;
    tDesc.Height = size->y;
    tDesc.MipLevels = 1;
    tDesc.ArraySize = 1;
    tDesc.Format = format; // DXGI_FORMAT_D24_UNORM_S8_UINT;
    tDesc.SampleDesc.Count = 1;
    tDesc.SampleDesc.Quality = 0;
    tDesc.Usage = D3D11_USAGE_DEFAULT;
    tDesc.BindFlags = flags; // D3D11_BIND_DEPTH_STENCIL;
    tDesc.CPUAccessFlags = 0;
    tDesc.MiscFlags = 0;
    DX11_GetDevice()->CreateTexture2D(&tDesc, NULL, buffer);

    if (resView) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = -1;
        HRESULT r = 0;
        r = DX11_GetDevice()->CreateShaderResourceView(*buffer, &srvDesc, resView);
        rgLogWarn(RG_LOG_RENDER, "ResView: %d, %lp", r, *resView);
    }
}

static void _CreateFramebuffer(ivec2* size) {
    _MakeTexture(&albedoBuffer, &resView[0], size, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    _MakeTexture(&normalBuffer, &resView[1], size, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    _MakeTexture(&wposBuffer,   &resView[2], size, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    _MakeTexture(&depthBuffer,  NULL,        size, DXGI_FORMAT_D24_UNORM_S8_UINT,  D3D11_BIND_DEPTH_STENCIL);

    DX11_GetDevice()->CreateRenderTargetView(albedoBuffer, NULL, &albedoView);
    DX11_GetDevice()->CreateRenderTargetView(normalBuffer, NULL, &normalView);
    DX11_GetDevice()->CreateRenderTargetView(wposBuffer,   NULL, &wposView);

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;
    DX11_GetDevice()->CreateDepthStencilView(depthBuffer, &depthStencilViewDesc, &depthStencilView);

}

static void _DestroyFramebuffer() {
    resView[0]->Release();
    resView[1]->Release();
    resView[2]->Release();

    albedoView->Release();
    normalView->Release();
    wposView->Release();
    depthStencilView->Release();

    albedoBuffer->Release();
    normalBuffer->Release();
    wposBuffer->Release();
    depthBuffer->Release();
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
        albedoView,
        normalView,
        wposView
    };
    ctx->OMSetRenderTargets(3, buffers, depthStencilView);
    ctx->OMSetDepthStencilState(depthStencilState, 1);
    ctx->RSSetState(rasterState);

    //ctx->ClearRenderTargetView(dx_backbuffer, clearColor);
    ctx->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0.0f);

}