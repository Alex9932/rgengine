#include <rshared.h>
#include <allocator.h>
#include <engine.h>
#include <event.h>
#include <render.h>

#include "rendertypesdx.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_impl_dx11.h"

using namespace Engine;

static Uint32 flags = 0;

Engine::Allocator* RGetAllocator() {
	return GetDefaultAllocator();
}

Uint32 RGetSetupFlags() {
	return flags;
}

SDL_Window* R_ShowWindow(Uint32 w, Uint32 h) {
	SDL_Window* sdl_hwnd = SDL_CreateWindow("rgEngine", w, h, 0);
	SDL_SetWindowPosition(sdl_hwnd, 5, 5);
	return sdl_hwnd;
}

void R_Setup() {
	//RG_ERROR_MSG("Direct3D 11 renderer is not support new rendering system!");
}

static IDXGIAdapter* SelectAdapter(RRenderDevice* device) {
	IDXGIAdapter* pAdapter = NULL;
	IDXGIFactory* pFactory = NULL;
	CreateDXGIFactory(IID_IDXGIFactory, (void**)&pFactory);
	DXGI_ADAPTER_DESC desc = {};

	// Use first adapter
	for (Uint32 i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
		pAdapter->GetDesc(&desc);
		SDL_snprintf(device->cardName, 128, "%ls", desc.Description);
		rgLogInfo(RG_LOG_RENDER, "Direct3D: %s", device->cardName);
		break;
	}
	pFactory->Release();
	return pAdapter;
}

#if R_DXRENDER_DEBUG
static void _PollMsg(ID3D11InfoQueue* infoqueue, UINT64 i) {
	SIZE_T messageSize = 0;
	infoqueue->GetMessage(i, nullptr, &messageSize);
	D3D11_MESSAGE* data = (D3D11_MESSAGE*)alloca(messageSize);
	infoqueue->GetMessageW(i, data, &messageSize);
	rgLogInfo(RG_LOG_RENDER, "DX11: %s", data->pDescription);
}

static void PollInfoQueue(RRenderDevice* device) {
	UINT64 messageCount = device->dxdbginfoqueue->GetNumStoredMessagesAllowedByRetrievalFilter();
	if (messageCount != 0) {
		rgLogInfo(RG_LOG_RENDER, "Direct3D Report:");
	}
	for (UINT64 i = 0; i < messageCount; i++) {
		_PollMsg(device->dxdbginfoqueue, i);
	}
	device->dxdbginfoqueue->ClearStoredMessages();
}
#endif

static Bool _EventHandler(SDL_Event* event, void* data) {
	RRenderDevice* device = (RRenderDevice*)data;


	if (event->type == Engine::GetUserEventID()) {
		switch (event->user.code) {
			case RG_EVENT_SYSTEM_SIGNAL: {
				Sint32 signal = (Sint32)event->user.data1;
#if R_DXRENDER_DEBUG
				PollInfoQueue(device);
#endif
				break;
			}
#if 0
			case RG_EVENT_RENDER_VIEWPORT_RESIZE: {
				ivec2* wnd_size = (ivec2*)event->user.data1;
				device->wndsize = *wnd_size;
				device->wndresized = true;
				rgLogWarn(RG_LOG_RENDER, "Size changed: %dx%d", wnd_size->x, wnd_size->y);
				break;
			}
#endif
		default: { break; }
		}

	}
	return true;
}

static RImage* GetBackBufferImage(RRenderDevice* dev, Uint32 i, Uint16 width, Uint16 height, RFormat format) {

	RImage* image = (RImage*)dev->allocator->Allocate(sizeof(RImage));
	image->dev = dev;
	image->format = format;
	image->width  = width;
	image->height = height;

	//HRESULT t = dev->dxswapchain->GetBuffer(info->var, IID_ID3D11Texture2D, (void**)&backbufferTex);
	HRESULT t = dev->dxswapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&image->image);
	RG_ASSERT_MSG(SUCCEEDED(t), "Unable to get swapchain buffers");

	//t = dev->dxdev->CreateRenderTargetView(backbufferTex, NULL, &rv->rtv);
	//RG_ASSERT_MSG(SUCCEEDED(t), "Unable create backbuffer view");
	//backbufferTex->Release();

	return image;
}

static void CreateDefaultFramebuffer(RRenderDevice* device) {

	RRenderpassCreateInfo rpinfo = {};
	//rpinfo.cullmode = RG_RENDERPASS_CULLMODE_NONE;
	//rpinfo.fillmode = RG_RENDERPASS_FILLMODE_SOLID;
	rpinfo.rt_count = 1;
	rpinfo.use_depth = false;
	rpinfo.viewport = { 0.0f, 0.0f, (Float32)device->wndsize.x, (Float32)device->wndsize.y };
	device->default_renderpass = R_CreateRenderpass(device, &rpinfo);
#if 1
	for (size_t i = 0; i < device->backbuffer_count; i++) {
		RFramebufferCreateInfo fbinfo = {};
		fbinfo.width      = (Uint16)device->wndsize.x;
		fbinfo.height     = (Uint16)device->wndsize.y;
		fbinfo.rt_count   = 1;
		fbinfo.rts[0]     = GetBackBufferImage(device, i, fbinfo.width, fbinfo.height, RG_FORMAT_R8G8B8A8_UNORM);// device->default_backbuffers[i];
		fbinfo.renderpass = device->default_renderpass;
		device->default_framebuffers[i] = R_CreateFramebuffer(device, &fbinfo);

	}
#endif
}

static void FreeDefaultFramebuffer(RRenderDevice* device) {
	for (size_t i = 0; i < device->backbuffer_count; i++) {
		R_DestroyFramebuffer(device->default_framebuffers[i]);
		//R_DestroyResourceView(device->default_backbuffers[i]);
	}
	R_DestroyRenderpass(device->default_renderpass);
}

RRenderDevice* R_CreateDevice(RRenderSetupInfo* info) {
	flags = info->flags;

	// Make new allocator for rendering device
	STDAllocator* alloc = RG_NEW(STDAllocator)("DX11 allocator");

	// Create device
	RRenderDevice* device = RG_NEW_CLASS(alloc, RRenderDevice);

	device->flags     = info->flags;
	device->allocator = alloc;
	device->hwnd      = info->hwnd;

	SDL_SetWindowTitle(device->hwnd, "rgEngine - D3D11");
	Engine::RegisterEventHandler(_EventHandler, device);

	SDL_PropertiesID props = SDL_GetWindowProperties(info->hwnd);
	HWND win_hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	int w = 0, h = 0;
	SDL_GetWindowSize(info->hwnd, &w, &h);
	device->wndsize.x = w;
	device->wndsize.y = h;

	IDXGIAdapter* pAdapter = SelectAdapter(device); // Use in future

	// Swapchain

	device->backbuffer_count = 2; // Buffers needed

	DXGI_SWAP_CHAIN_DESC scd = {};
	//ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
	scd.BufferCount        = 1;
	scd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow       = win_hwnd;
	scd.SampleDesc.Count   = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed           = TRUE;
	scd.OutputWindow	   = win_hwnd;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
	//scd.BufferDesc.Width   = w;
	//scd.BufferDesc.Height  = h;
	scd.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags              = 0;

	D3D_FEATURE_LEVEL levels[] = {
	   D3D_FEATURE_LEVEL_11_0,
	   D3D_FEATURE_LEVEL_11_1
	};
	UINT flags = 0;
#if R_DXRENDER_DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, 2, D3D11_SDK_VERSION, &scd, &device->dxswapchain, &device->dxdev, NULL, &device->dxctx);

	// Get actual backbuffer count
	DXGI_SWAP_CHAIN_DESC scDesc;
	device->dxswapchain->GetDesc(&scDesc);
	device->backbuffer_count = scDesc.BufferCount;

#if R_DXRENDER_DEBUG
	rgLogInfo(RG_LOG_RENDER, "Setup query");
	device->dxdev->QueryInterface(__uuidof(ID3D11Debug), (void**)&device->dxdbg);
	device->dxdev->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&device->dxdbginfoqueue);

	//device->dxdbginfoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	//device->dxdbginfoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);

	//ID3D11InfoQueue* infoQueue = nullptr;
	//if (SUCCEEDED(device->dxdev->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&infoQueue))) {
	//	infoQueue->SetMessageCountLimit(0);
	//	infoQueue->Release();
	//}

#endif

	RBufferCreateInfo pc_info = {};
	pc_info.access = RG_BUFFER_ACCESS_CPU_WRITE;
	pc_info.length = 128;
	pc_info.type   = RG_BUFFER_TYPE_CONSTANT;
	pc_info.usage  = RG_BUFFER_USAGE_DYNAMIC;
	device->pushconstant = R_CreateBuffer(device, &pc_info);
	//device->pc_pixel  = R_CreateBuffer(device, &pc_info);

	RG_ASSERT_MSG(device->dxdev, "Unable to initialize direct3d: D3D11Device");
	RG_ASSERT_MSG(device->dxctx, "Unable to initialize direct3d: D3D11DeviceContext");
	RG_ASSERT_MSG(device->dxswapchain, "Unable to initialize direct3d: D3D11SwapChain");

	device->wndresized = false;
	device->currentframe = 0;

	CreateDefaultFramebuffer(device);

	//device->dxdbg->ValidateContext(device->dxctx);

	SDL_SetWindowPosition(device->hwnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	return device;
}

void R_DestroyDevice(RRenderDevice* device) {

	Engine::FreeEventHandler(_EventHandler);

	R_DestroyBuffer(device->pushconstant);
	//R_DestroyBuffer(device->pc_pixel);

#if R_DXRENDER_DEBUG
	device->dxdbginfoqueue->Release();
	device->dxdbg->Release();
#endif

	FreeDefaultFramebuffer(device);

	device->dxswapchain->Release();
	device->dxctx->Release();
	device->dxdev->Release();

	// Free device object
	STDAllocator* alloc = (STDAllocator*)device->allocator;
	RG_DELETE_CLASS(alloc, RRenderDevice, device);
	// And delete allocator
	RG_DELETE(STDAllocator, alloc);
}

void R_WaitIdle(RRenderDevice* device) {
	D3D11_QUERY_DESC queryDesc;
	queryDesc.Query = D3D11_QUERY_EVENT;
	queryDesc.MiscFlags = 0;

	ID3D11Query* pEventQuery = NULL;
	device->dxdev->CreateQuery(&queryDesc, &pEventQuery);
	device->dxctx->End(pEventQuery);
	device->dxctx->Flush();

	while (device->dxctx->GetData(pEventQuery, NULL, 0, 0) == S_FALSE) {
		//Yield();
		Sleep(1);
	}

	pEventQuery->Release();
}

static Bool firstswap = true;
void R_SwapBuffers(RRenderDevice* device, RSwapBuffersInfo* info) {

	if (!RG_CHECK_FLAG(info->flags, RG_SWAPCHAIN_FLAG_RESIZE)) {
		device->dxswapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);

		device->currentframe++;
		device->currentframe = device->currentframe % device->backbuffer_count;
	}

	// Resize swapchain
	if (RG_CHECK_FLAG(info->flags, RG_SWAPCHAIN_FLAG_RESIZE)) {

		device->wndsize = info->newsize;
		device->currentframe = 0;

		device->dxctx->ClearState();
		device->dxctx->Flush();

		rgLogInfo(RG_LOG_RENDER, "Swapchain resized %dx%d", device->wndsize.x, device->wndsize.y);
		FreeDefaultFramebuffer(device);

		HRESULT result = device->dxswapchain->ResizeBuffers(0, device->wndsize.x, device->wndsize.y, DXGI_FORMAT_UNKNOWN, 0);
		RG_ASSERT_MSG(SUCCEEDED(result), "Unable to resize swapchain buffers");

		CreateDefaultFramebuffer(device);

		device->wndresized = false;
	}

#if R_DXRENDER_DEBUG
	if (firstswap) {
		firstswap = false;
		PollInfoQueue(device);
	}
#endif
}

void R_GetInfo(RRenderDevice* dev, RenderInfo* info) {
	info->render_name = R_RENDERER_NAME;
	info->renderer    = dev->cardName;
}

//////////////////////////////////////////////////////////
// IMGUI
//////////////////////////////////////////////////////////

void R_ImGui_Init(RRenderDevice* dev) {
	ImGui_ImplDX11_Init(dev->dxdev, dev->dxctx);
}

void R_ImGui_Shutdown(RRenderDevice* dev) {
	ImGui_ImplDX11_Shutdown();
}

void R_ImGui_NewFrame(RRenderDevice* dev) {
	ImGui_ImplDX11_NewFrame();
}

void* R_ImGui_AddTexture(RRenderDevice* dev, RImage* image) {
	ID3D11ShaderResourceView* srv = NULL;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = GetFormat(image->format);
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	dev->dxdev->CreateShaderResourceView(image->image, &srvDesc, &srv);
	//dev->dxctx->GenerateMips(srv);

	return srv;
}

void R_ImGui_RemoveTexture(void* handle) {
	if (!handle) { return; }
	ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)handle;
	srv->Release();
}
