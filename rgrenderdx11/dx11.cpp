#include "dx11.h"

#include <SDL2/SDL_syswm.h>
#include <engine.h>
#include <rgvector.h>

static ID3D11Device*        dx_device;
static ID3D11DeviceContext* dx_ctx;
static IDXGISwapChain*      dx_swapchain;

// Color buffer
static ID3D11RenderTargetView* dx_backbuffer;

static ID3D11RasterizerState*  dx_rasterState;

static ID3D11BlendState*       dx_blendState;

static char gCardName[128] = { 0 };

static void _MakeRendertarget() {
    // Render target
    ID3D11Texture2D* pBackBuffer = NULL;
    dx_swapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&pBackBuffer);
    dx_device->CreateRenderTargetView(pBackBuffer, NULL, &dx_backbuffer);
    pBackBuffer->Release();
}

void DX11_Initialize(SDL_Window* hwnd) {
    // Get HWND
    SDL_SysWMinfo wminfo = {};
    SDL_VERSION(&wminfo.version);
    SDL_GetWindowWMInfo(hwnd, &wminfo);
    HWND win_hwnd = (HWND)wminfo.info.win.window;

    int w = 0, h = 0;
    SDL_GetWindowSize(hwnd, &w, &h);

    IDXGIFactory* pFactory = NULL;
    IDXGIAdapter* pAdapter = NULL;
    CreateDXGIFactory(IID_IDXGIFactory, (void**)&pFactory);
    DXGI_ADAPTER_DESC desc = {};
    for (Uint32 i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
        pAdapter->GetDesc(&desc);
        SDL_snprintf(gCardName, 128, "%ls", desc.Description);
        rgLogInfo(RG_LOG_RENDER, "Direct3D: %s", gCardName);
        break;
    }
    pFactory->Release();

    // Swapchain
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = win_hwnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags = 0;

    D3D_FEATURE_LEVEL levels[] = {
       D3D_FEATURE_LEVEL_11_0,
       D3D_FEATURE_LEVEL_11_1
    };
    UINT flags = 0;
#if DX_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, 2, D3D11_SDK_VERSION, &scd, &dx_swapchain, &dx_device, NULL, &dx_ctx);

    RG_ASSERT_MSG(dx_device, "Unable to initialize direct3d: D3D11Device");
    RG_ASSERT_MSG(dx_ctx, "Unable to initialize direct3d: D3D11DeviceContext");
    RG_ASSERT_MSG(dx_swapchain, "Unable to initialize direct3d: D3D11SwapChain");

    _MakeRendertarget();
    DX11_SetViewport(w, h);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.AntialiasedLineEnable = false;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = false;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.ScissorEnable = false;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    dx_device->CreateRasterizerState(&rasterDesc, &dx_rasterState);

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = true;

    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    /*
    blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
    */
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    DX11_GetDevice()->CreateBlendState(&blendDesc, &dx_blendState);
}

void DX11_Destroy() {
    dx_blendState->Release();
    dx_rasterState->Release();
    dx_swapchain->Release();
    dx_backbuffer->Release();
    dx_device->Release();
    dx_ctx->Release();
}

void DX11_BindDefaultFramebuffer() {

    dx_ctx->OMSetRenderTargets(1, &dx_backbuffer, NULL);
    dx_ctx->RSSetState(dx_rasterState);

    Float32 blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    dx_ctx->OMSetBlendState(dx_blendState, blendFactor, 0xffffffff);

}

void DX11_SwapBuffers() {
    dx_swapchain->Present(0, 0);
}

void DX11_Clear(Float32* clearColor) {
    dx_ctx->ClearRenderTargetView(dx_backbuffer, clearColor);
}

void DX11_Resize(ivec2* wndSize) {
    dx_backbuffer->Release();
    dx_swapchain->ResizeBuffers(0, wndSize->x, wndSize->y, DXGI_FORMAT_UNKNOWN, 0);
    _MakeRendertarget();
    DX11_SetViewport(wndSize->x, wndSize->y);
}

IDXGISwapChain*      DX11_GetSwapchain()        { return dx_swapchain; }
ID3D11Device*        DX11_GetDevice()           { return dx_device; }
ID3D11DeviceContext* DX11_GetContext()          { return dx_ctx; }

String               DX11_GetGraphicsCardName() { return gCardName; }

void DX11_MakeTexture(ID3D11Texture2D** buffer, ID3D11ShaderResourceView** resView, ivec2* size, DXGI_FORMAT format, UINT flags) {

    D3D11_TEXTURE2D_DESC tDesc = {};
    tDesc.Width = size->x;
    tDesc.Height = size->y;
    tDesc.MipLevels = 1;
    tDesc.ArraySize = 1;
    tDesc.Format = format;
    tDesc.SampleDesc.Count = 1;
    tDesc.SampleDesc.Quality = 0;
    tDesc.Usage = D3D11_USAGE_DEFAULT;
    tDesc.BindFlags = flags;
    tDesc.CPUAccessFlags = 0;
    tDesc.MiscFlags = 0;
    dx_device->CreateTexture2D(&tDesc, NULL, buffer);

    if (resView) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = -1;
        HRESULT r = 0;
        r = dx_device->CreateShaderResourceView(*buffer, &srvDesc, resView);
        //rgLogWarn(RG_LOG_RENDER, "ResView: %d, %lp", r, *resView);
    }
}

void DX11_MakeRenderTarget(RenderTarget* rt, ivec2* size, DXGI_FORMAT format) {
    DX11_MakeTexture(&rt->texture, &rt->resView, size, format, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    DX11_GetDevice()->CreateRenderTargetView(rt->texture, NULL, &rt->rtView);
}

void DX11_FreeRenderTarget(RenderTarget* rt) {
    rt->resView->Release();
    rt->rtView->Release();
    rt->texture->Release();
}

void DX11_SetViewport(int w, int h) {
    D3D11_VIEWPORT pViewport = {};
    pViewport.TopLeftX = 0;
    pViewport.TopLeftY = 0;
    pViewport.Width    = (Float32)w;
    pViewport.Height   = (Float32)h;
    pViewport.MinDepth = 0.0f;
    pViewport.MaxDepth = 1.0f;
    dx_ctx->RSSetViewports(1, &pViewport);
}