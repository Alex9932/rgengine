#ifndef _DX11_H
#define _DX11_H

#include <d3d11.h>
#include "rgtypes.h"
#include <rgvector.h>

#define DX_DEBUG 0

typedef struct RenderTarget {
	ID3D11Texture2D*          texture;
	ID3D11ShaderResourceView* resView;
	ID3D11RenderTargetView*   rtView;
} RenderTarget;

void DX11_Initialize(SDL_Window* hwnd);
void DX11_Destroy();

void DX11_BindDefaultFramebuffer();
void DX11_SwapBuffers();

void DX11_Clear(Float32* clearColor);
void DX11_Resize(ivec2* wndSize);

IDXGISwapChain* DX11_GetSwapchain();
ID3D11Device* DX11_GetDevice();
ID3D11DeviceContext* DX11_GetContext();
String DX11_GetGraphicsCardName();

void DX11_MakeTexture(ID3D11Texture2D** buffer, ID3D11ShaderResourceView** resView, ivec2* size, DXGI_FORMAT format, UINT flags);

void DX11_MakeRenderTarget(RenderTarget* rt, ivec2* size, DXGI_FORMAT format);
void DX11_FreeRenderTarget(RenderTarget* rt);

void DX11_SetViewport(int w, int h);

#endif