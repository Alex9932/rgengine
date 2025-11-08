#include <rshared.h>
#include <allocator.h>
#include <engine.h>

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

static D3D11_USAGE GetBufferUsage(Uint8 usage) {
	switch (usage) {
	case RG_BUFFER_USAGE_DEFAULT: return D3D11_USAGE_DEFAULT;
	case RG_BUFFER_USAGE_DYNAMIC: return D3D11_USAGE_DYNAMIC;
	default: return D3D11_USAGE_DEFAULT;
	}
}

static UINT GetBufferCPUAccess(Uint8 access) {
	UINT cpuAccessFlags = 0;
	if (access & RG_BUFFER_ACCESS_CPU_WRITE) cpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;
	if (access & RG_BUFFER_ACCESS_CPU_READ)  cpuAccessFlags |= D3D11_CPU_ACCESS_READ;
	return cpuAccessFlags;
}

static UINT GetBufferType(Uint16 type) {
	UINT bindFlags = 0;
	if (type & RG_BUFFER_TYPE_VERTEX)     bindFlags |= D3D11_BIND_VERTEX_BUFFER;
	if (type & RG_BUFFER_TYPE_INDEX)      bindFlags |= D3D11_BIND_INDEX_BUFFER;
	if (type & RG_BUFFER_TYPE_CONSTANT)   bindFlags |= D3D11_BIND_CONSTANT_BUFFER;
	if (type & RG_BUFFER_TYPE_SHADER_RES) bindFlags |= D3D11_BIND_SHADER_RESOURCE;
	if (type & RG_BUFFER_TYPE_UNORDERED)  bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	return bindFlags;
}

static UINT GetBufferMiscFlags(Uint16 type) {
	UINT miscFlags = 0;
	if (type & RG_BUFFER_TYPE_STRUCTURED) miscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	return miscFlags;
}

SDL_Window* R_ShowWindow(Uint32 w, Uint32 h) {
	SDL_Window* sdl_hwnd = SDL_CreateWindow("rgEngine", w, h, 0);
	SDL_SetWindowPosition(sdl_hwnd, 5, 5);
	return sdl_hwnd;
}

void R_Setup() {

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

RRenderDevice* R_CreateDevice(RRenderSetupInfo* info) {
	flags = info->flags;

	// Make new allocator for rendering device
	STDAllocator* alloc = RG_NEW(STDAllocator)("DX11 allocator");

	// Create device
	RRenderDevice* device = RG_NEW_CLASS(alloc, RRenderDevice);

	SDL_PropertiesID props = SDL_GetWindowProperties(info->hwnd);
	HWND win_hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	int w = 0, h = 0;
	SDL_GetWindowSize(info->hwnd, &w, &h);

	IDXGIAdapter* pAdapter = SelectAdapter(device); // Use in future

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
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, 2, D3D11_SDK_VERSION, &scd, &device->dxswapchain, &device->dxdev, NULL, &device->dxctx);

	RG_ASSERT_MSG(device->dxdev, "Unable to initialize direct3d: D3D11Device");
	RG_ASSERT_MSG(device->dxctx, "Unable to initialize direct3d: D3D11DeviceContext");
	RG_ASSERT_MSG(device->dxswapchain, "Unable to initialize direct3d: D3D11SwapChain");

	device->flags     = info->flags;
	device->allocator = alloc;

	return device;
}

void R_DestroyDevice(RRenderDevice* device) {



	// Free device object
	STDAllocator* alloc = (STDAllocator*)device->allocator;
	RG_DELETE_CLASS(alloc, RRenderDevice, device);
	// And delete allocator
	RG_DELETE(STDAllocator, alloc);
}

void R_SwapBuffers(RRenderDevice* device) {
	device->dxswapchain->Present(0, 0);
}

void R_GetInfo(RRenderDevice* dev, RenderInfo* info) {
	
}

void R_ImGui_Init(RRenderDevice* dev) {
	ImGui_ImplDX11_Init(dev->dxdev, dev->dxctx);
}

void R_ImGui_Shutdown(RRenderDevice* dev) {
	ImGui_ImplDX11_Shutdown();
}

void R_ImGui_NewFrame(RRenderDevice* dev) {
	ImGui_ImplDX11_NewFrame();
}

void R_ImGui_RenderDrawData(RRenderDevice* dev, void* drawData) {
	ImGui_ImplDX11_RenderDrawData((ImDrawData*)drawData);
}


RBuffer* R_CreateBuffer(RRenderDevice* dev, RBufferCreateInfo* info) {
	RBuffer* buffer = (RBuffer*)dev->allocator->Allocate(sizeof(RBuffer));
	buffer->dev = dev;

	buffer->access = info->access;
	buffer->usage  = info->usage;
	buffer->type   = info->type;
	buffer->length = info->length;

	// Make buffer
	D3D11_BUFFER_DESC buff = {};
	buff.Usage          = GetBufferUsage(buffer->usage);
	buff.ByteWidth      = buffer->length;
	buff.BindFlags      = GetBufferType(buffer->type);
	buff.CPUAccessFlags = GetBufferCPUAccess(buffer->access);
	buff.MiscFlags      = GetBufferMiscFlags(buffer->type);
	buff.StructureByteStride = info->stride;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem          = info->initialData;
	data.SysMemPitch      = 1;
	data.SysMemSlicePitch = info->length;

	D3D11_SUBRESOURCE_DATA* dataptr = NULL;
	if (info->initialData) {
		dataptr = &data;
	}

	HRESULT result = dev->dxdev->CreateBuffer(&buff, dataptr, &buffer->buffer);
	RG_ASSERT_MSG(buffer->buffer, "Unable to create buffer: D3D11Buffer");
	dev->buffersMemLen += buffer->length;
	return buffer;
}

void R_DestroyBuffer(RBuffer* buffer) {
	RRenderDevice* dev = buffer->dev;
	buffer->buffer->Release();
	dev->buffersMemLen -= buffer->length;
	dev->allocator->Deallocate(buffer);
}

RImage* R_CreateImage(RRenderDevice* dev, RImageCreateInfo* info) {
	RImage* image = (RImage*)dev->allocator->Allocate(sizeof(RImage));
	image->dev = dev;

	// Make image

	return image;
}

void R_DestroyImage(RImage* image) {
	RRenderDevice* dev = image->dev;

	// Free image

	dev->allocator->Deallocate(image);
}

RCommandBuffer* R_CreateCommandBuffer(RRenderDevice* dev, RCommandBufferCreateInfo* info) {
	RCommandBuffer* cmdbuff = (RCommandBuffer*)dev->allocator->Allocate(sizeof(RCommandBuffer));
	cmdbuff->dev = dev;

	// Make queue

	return cmdbuff;
}

void R_DestroyCommandBuffer(RCommandBuffer* cmdbuff) {
	RRenderDevice* dev = cmdbuff->dev;

	// Free queue

	dev->allocator->Deallocate(cmdbuff);
}

void R_SubmitCommandBuffer(RCommandBufferSubmitInfo* info) {

}