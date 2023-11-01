#include "dx11.h"

#include <SDL2/SDL_syswm.h>
#include <engine.h>

static ID3D11Device* dx_device;
static ID3D11DeviceContext* dx_ctx;
static IDXGISwapChain* dx_swapchain;

// Color buffer
static ID3D11RenderTargetView* dx_backbuffer;

// Depth-stencil buffer
static ID3D11Texture2D* dx_depthbuffer;
static ID3D11DepthStencilState* dx_depthStencilState;
static ID3D11DepthStencilView* dx_depthStencilView;

static ID3D11RasterizerState* dx_rasterState;

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
        rgLogInfo(RG_LOG_RENDER, "Direct3D: %ls", desc.Description);
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

    // Render target
    ID3D11Texture2D* pBackBuffer = NULL;
    dx_swapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&pBackBuffer);
    dx_device->CreateRenderTargetView(pBackBuffer, NULL, &dx_backbuffer);
    pBackBuffer->Release();

#if 0
    //dx_device->CreateRenderTargetView(pDepthBuffer, NULL, &dx_depthbuffer);
    ID3D11Texture2D* pDepthBuffer = NULL;

    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    depthBufferDesc.Width = w;
    depthBufferDesc.Height = h;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;
    dx_device->CreateTexture2D(&depthBufferDesc, NULL, &dx_depthbuffer);

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    // Set up the description of the stencil state.
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing.
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing.
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dx_device->CreateDepthStencilState(&depthStencilDesc, &dx_depthStencilState);


    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    // Set up the depth stencil view description.
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    // Create the depth stencil view.
    dx_device->CreateDepthStencilView(dx_depthbuffer, &depthStencilViewDesc, &dx_depthStencilView);
#endif
    //dx_ctx->OMSetRenderTargets(1, &dx_backbuffer, NULL);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.AntialiasedLineEnable = false;
    //rasterDesc.CullMode = D3D11_CULL_FRONT;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = false;
    //rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.ScissorEnable = false;
    rasterDesc.SlopeScaledDepthBias = 0.0f;

    // Create the rasterizer state from the description we just filled out.
    dx_device->CreateRasterizerState(&rasterDesc, &dx_rasterState);


    // Viewport
    D3D11_VIEWPORT pViewport;
    ZeroMemory(&pViewport, sizeof(D3D11_VIEWPORT));
    pViewport.TopLeftX = 0;
    pViewport.TopLeftY = 0;
    pViewport.Width = w;
    pViewport.Height = h;
    pViewport.MinDepth = 0.0f;
    pViewport.MaxDepth = 1.0f;
    dx_ctx->RSSetViewports(1, &pViewport);
}

void DX11_Destroy() {
    dx_swapchain->Release();
    dx_backbuffer->Release();
    //dx_depthbuffer->Release();
    //dx_depthStencilView->Release();
    //dx_depthStencilState->Release();
    dx_device->Release();
    dx_ctx->Release();
}

void DX11_BindDefaultFramebuffer() {

    //dx_ctx->OMSetDepthStencilState(dx_depthStencilState, 1);
    dx_ctx->OMSetRenderTargets(1, &dx_backbuffer, dx_depthStencilView);
    dx_ctx->RSSetState(dx_rasterState);

}

void DX11_SwapBuffers() {
    dx_swapchain->Present(0, 0);
}

void DX11_Clear(Float32* clearColor) {
    dx_ctx->ClearRenderTargetView(dx_backbuffer, clearColor);
    //dx_ctx->ClearDepthStencilView(dx_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0.0f);
}

IDXGISwapChain*      DX11_GetSwapchain() { return dx_swapchain; }
ID3D11Device*        DX11_GetDevice()    { return dx_device; }
ID3D11DeviceContext* DX11_GetContext()   { return dx_ctx; }