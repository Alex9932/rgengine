#include <rshared.h>
#include <allocator.h>
#include <engine.h>
#include <event.h>
#include <render.h>

#include "rendertypesdx.h"

#define R_DXRENDER_DEBUG 1

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

static Bool _EventHandler(SDL_Event* event, void* data) {
	RRenderDevice* device = (RRenderDevice*)data;
#if 0
	if (event->type == Engine::GetUserEventID()) {

		switch (event->user.code) {
			case RG_EVENT_RENDER_VIEWPORT_RESIZE: {
				ivec2* wnd_size = (ivec2*)event->user.data1;
				device->wndsize = *wnd_size;
				device->wndresized = true;
				rgLogWarn(RG_LOG_RENDER, "Size changed: %dx%d", wnd_size->x, wnd_size->y);
				break;
			}
		default: { break; }
		}

	}
#endif
	return true;
}

RRenderDevice* R_CreateDevice(RRenderSetupInfo* info) {
	flags = info->flags;

	// Make new allocator for rendering device
	STDAllocator* alloc = RG_NEW(STDAllocator)("DX11 allocator");

	// Create device
	RRenderDevice* device = RG_NEW_CLASS(alloc, RRenderDevice);
	device->hwnd = info->hwnd;
	SDL_SetWindowTitle(device->hwnd, "rgEngine - D3D11");
	Engine::RegisterEventHandler(_EventHandler, device);

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

	// Get swapchain backbuffer. Use only first
	device->dxswapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&device->backbuffers[0]);
	device->wndresized = false;


	SDL_SetWindowPosition(device->hwnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	return device;
}

void R_DestroyDevice(RRenderDevice* device) {



	// Free device object
	STDAllocator* alloc = (STDAllocator*)device->allocator;
	RG_DELETE_CLASS(alloc, RRenderDevice, device);
	// And delete allocator
	RG_DELETE(STDAllocator, alloc);
}

void R_SwapBuffers(RRenderDevice* device, RSwapBuffersInfo* info) {

	if (!RG_CHECK_FLAG(info->flags, RG_SWAPCHAIN_FLAG_RESIZE)) {
		device->dxswapchain->Present(0, 0);
	}

	// Resize swapchain
	if (RG_CHECK_FLAG(info->flags, RG_SWAPCHAIN_FLAG_RESIZE)) {

		device->wndsize = info->newsize;

		device->dxctx->ClearState();
		device->dxctx->Flush();

		device->backbuffers[0]->Release();
		HRESULT result = device->dxswapchain->ResizeBuffers(0, device->wndsize.x, device->wndsize.y, DXGI_FORMAT_UNKNOWN, 0);
		RG_ASSERT_MSG(SUCCEEDED(result), "Unable to resize swapchain buffers");
		device->dxswapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&device->backbuffers[0]);
	}
}

void R_GetInfo(RRenderDevice* dev, RenderInfo* info) {
	info->render_name = "Direct3D 11";
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

//////////////////////////////////////////////////////////
// BUFFER
//////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////
// IMAGE
//////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////
// COMMAND BUFFER
//////////////////////////////////////////////////////////

RCommandBuffer* R_CreateCommandBuffer(RRenderDevice* dev, RCommandBufferCreateInfo* info) {
	RCommandBuffer* cmdbuff = (RCommandBuffer*)dev->allocator->Allocate(sizeof(RCommandBuffer));
	cmdbuff->dev = dev;
	char allocname[64];
	SDL_snprintf(allocname, 64, "DX11 Commandpool (0x%p)", cmdbuff);
	if (info->maxcmds == 0) { info->maxcmds = R_MAX_COMMANDS_PER_BUFFER; }
	cmdbuff->pool = RG_NEW_CLASS(dev->allocator, LinearAllocator)(allocname, info->maxcmds *sizeof(RCommand));
	cmdbuff->commands_recorded = 0;
	return cmdbuff;
}

void R_DestroyCommandBuffer(RCommandBuffer* cmdbuff) {
	RRenderDevice* dev = cmdbuff->dev;
	cmdbuff->pool->Deallocate();
	RG_DELETE_CLASS(dev->allocator, LinearAllocator, cmdbuff->pool);
	dev->allocator->Deallocate(cmdbuff);
}

void R_ResetCommandBuffer(RCommandBuffer* cmdbuff) {
	cmdbuff->commands_recorded = 0;
	cmdbuff->pool->Deallocate();
}

void R_BeginCommandBuffer(RCommandBuffer* buffer) { /* DO NOTHING */ }
void R_EndCommandBuffer(RCommandBuffer* buffer)   { /* DO NOTHING */ }

static inline DXGI_FORMAT GetIndexType(IndexType type) {
	switch (type) {
	case RG_INDEX_U8:  return DXGI_FORMAT_R8_UINT;
	case RG_INDEX_U16: return DXGI_FORMAT_R16_UINT;
	case RG_INDEX_U32: return DXGI_FORMAT_R32_UINT;
	default:           return DXGI_FORMAT_R8_UINT;
	}
}

static RG_INLINE void CMD_BeginRenderpassImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RRenderpass* rp = (RRenderpass*)cmd->handle;
	RRenderDevice* dev = buffer->dev;

	// Set render targets, clear, etc.

	dev->dxctx->OMSetRenderTargets(rp->rtv_count, rp->rtv, rp->dsv);
	dev->dxctx->OMSetDepthStencilState(rp->depth_stencil_state, 1);
	dev->dxctx->RSSetState(rp->raster_state);

	Float32 blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	dev->dxctx->OMSetBlendState(rp->blend_state, blendFactor, 0xffffffff);
}

static RG_INLINE void CMD_EndRenderpassImpl(RCommandBuffer* buffer, RCommand* cmd) {

}

static RG_INLINE void CMD_BindPipelineImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RPipeline* pl = (RPipeline*)cmd->handle;
	// Bind pipeline
}

static RG_INLINE void CMD_BindVertexBufferImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RBuffer* vb = (RBuffer*)cmd->handle;
	Uint32 slot = cmd->data0;
	UINT stride = cmd->data1;
	UINT offset = cmd->data2;
	buffer->dev->dxctx->IASetVertexBuffers(slot, 1, &vb->buffer, &stride, &offset);
}

static RG_INLINE void CMD_BindIndexBufferImpl(RCommandBuffer* buffer, RCommand* cmd) {
	RBuffer* ib = (RBuffer*)cmd->handle;
	IndexType indexFormat = (IndexType)cmd->data0;  // Index size in bytes
	buffer->dev->dxctx->IASetIndexBuffer(ib->buffer, GetIndexType(indexFormat), 0);
}

static RG_INLINE void CMD_DrawImGuiImpl(RCommandBuffer* buffer, RCommand* cmd) {
	// Restore pointer
	void* rawpointer = (void*)(((Uint64)cmd->data0 << 32) | (Uint64)cmd->data1);
	ImGui_ImplDX11_RenderDrawData((ImDrawData*)rawpointer);
}

static RG_INLINE void CMD_DrawIndexdImpl(RCommandBuffer* buffer, RCommand* cmd) {
	Uint32 idxcount = cmd->data0;
	Uint32 idxstart = cmd->data1;
	buffer->dev->dxctx->DrawIndexed(idxcount, idxstart, 0);
}

void R_SubmitCommandBuffer(RCommandBufferSubmitInfo* info) {
	RCommandBuffer* buffer = info->buffer;

	// Execute commands

	for (Uint32 i = 0; i < buffer->commands_recorded; i++) {
		RCommand* cmd = (RCommand*)((Uint8*)buffer->pool->GetBasePointer() + i * sizeof(RCommand));
		switch (cmd->cmd) {
			case R_CMD_NOP: { break; }
			case R_CMD_BEGIN_RENDERPASS:   { CMD_BeginRenderpassImpl(buffer, cmd); break; }
			case R_CMD_END_RENDERPASS:     { CMD_EndRenderpassImpl(buffer, cmd); break; }
			case R_CMD_BIND_PIPELINE:      { CMD_BindPipelineImpl(buffer, cmd); break; }
			case R_CMD_BIND_VERTEX_BUFFER: { CMD_BindVertexBufferImpl(buffer, cmd); break; }
			case R_CMD_BIND_INDEX_BUFFER:  { CMD_BindIndexBufferImpl(buffer, cmd); break; }
			case R_CMD_DRAW_IMGUI:         { CMD_DrawImGuiImpl(buffer, cmd); break; }
			//case R_CMD_DRAW:               { CMD_DrawImpl(buffer, cmd); break; }
			case R_CMD_DRAW_INDEXED:       { CMD_DrawIndexdImpl(buffer, cmd); break; }
			default: {
#if R_DXRENDER_DEBUG
				rgLogError(RG_LOG_RENDER, "DX11 Renderer: Invalid or unimplemented opcode in command buffer!");
#endif
				break;
			}
		}
	}

}

///////////
// COMMANDS

static RG_INLINE RCommand* AllocateNextCommand(RCommandBuffer* cmdbuff) {
	RCommand* cmd = (RCommand*)cmdbuff->pool->Allocate(sizeof(RCommand));
	cmdbuff->commands_recorded++;
	return cmd;
}

void R_CmdBeginRenderpass(RCommandBuffer* cmdbuff, RRenderpass* rp) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd    = R_CMD_BEGIN_RENDERPASS;
	cmd->handle = rp;
}

void R_CmdEndRenderpass(RCommandBuffer* cmdbuff) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_END_RENDERPASS;
}

void R_CmdBindPipeline(RCommandBuffer* cmdbuff, RPipeline* pl) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd    = R_CMD_BIND_PIPELINE;
	cmd->handle = pl;
}

void R_CmdBindVertexBuffer(RCommandBuffer* cmdbuff, RBuffer* vb, Uint32 slot) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd    = R_CMD_BIND_VERTEX_BUFFER;
	cmd->handle = vb;
	cmd->data0  = slot; // Bind slot
	cmd->data1  = 0;    // Stride
	cmd->data2  = 0;    // Offset
}

void R_CmdBindIndexBuffer(RCommandBuffer* cmdbuff, RBuffer* ib, IndexType isize) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd    = R_CMD_BIND_INDEX_BUFFER;
	cmd->handle = ib;
	cmd->data0  = isize;
}

/*
void R_CmdDraw(RCommandBuffer* cmdbuff, Uint32 idxcount, Uint32 idxstart) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_DRAW;
	cmd->data0 = idxcount;
	cmd->data1 = idxstart;
}
*/

void R_CmdDrawIndexed(RCommandBuffer* cmdbuff, Uint32 idxcount, Uint32 idxstart) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_DRAW_INDEXED;
	cmd->data0 = idxcount;
	cmd->data1 = idxstart;
}

void R_CmdImGuiRenderDrawData(RCommandBuffer* cmdbuff, void* data) {
	RCommand* cmd = AllocateNextCommand(cmdbuff);
	cmd->cmd = R_CMD_DRAW_IMGUI;

	Uint64 address = (Uint64)data;
	cmd->data0 = (Uint32)(address >> 32); // High part
	cmd->data1 = (Uint32)(address & 0xFFFFFFFF); // Low part
}


//////////////////////////////////////////////////////////
// RESOURCE VIEW
//////////////////////////////////////////////////////////

RResourceView* R_CreateResourceView(RRenderDevice* dev, RResourceViewCreateInfo* info) {
	RResourceView* rv = (RResourceView*)dev->allocator->Allocate(sizeof(RResourceView));
	rv->dev = dev;

	// Backbuffer view
	if (info->type == RG_RESOURCEVIEW_TYPE_BBV) {
		dev->dxdev->CreateRenderTargetView(dev->backbuffers[info->var], NULL, &rv->rtv);
		rv->type = R_DX_RESOURCEVIEW_RTV;
	}

	// Make rv

	return rv;
}

void R_DestroyResourceView(RResourceView* rv) {
	RRenderDevice* dev = rv->dev;

	if (rv->type == R_DX_RESOURCEVIEW_RTV) {
		rv->rtv->Release();
	}

	// Free rv

	dev->allocator->Deallocate(rv);
}

RRenderpass* R_CreateRenderpass(RRenderDevice* dev, RRenderpassCreateInfo* info) {
	RRenderpass* rp = (RRenderpass*)dev->allocator->Allocate(sizeof(RRenderpass));
	rp->dev = dev;

	rp->rtv_count = info->rt_count;
	rp->dsv = NULL;
	rp->depth_stencil_state = NULL;

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;

	// TODO: check this resourceviews
	for (Uint32 i = 0; i < rp->rtv_count; i++) {
#if R_DXRENDER_DEBUG
		if (info->rts[i]->type != R_DX_RESOURCEVIEW_RTV) {
			rgLogError(RG_LOG_RENDER, "DX11 Renderer: RResourceView->type(rts[%d]) must be a RG_RESOURCEVIEW_TYPE_RTV or RG_RESOURCEVIEW_TYPE_BBV in RRenderpass creation!", i);
		}
#endif

		blendDesc.RenderTarget[i].BlendEnable = true;
		blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		rp->rtv[i] = info->rts[i]->rtv;
	}

	dev->dxdev->CreateBlendState(&blendDesc, &rp->blend_state);

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.AntialiasedLineEnable = false;
	switch (info->cullmode) {
		case RG_RENDERPASS_CULLMODE_NONE:  { rasterDesc.CullMode = D3D11_CULL_NONE;  break; }
		case RG_RENDERPASS_CULLMODE_FRONT: { rasterDesc.CullMode = D3D11_CULL_FRONT; break; }
		case RG_RENDERPASS_CULLMODE_BACK:  { rasterDesc.CullMode = D3D11_CULL_BACK;  break; }
		default: { rasterDesc.CullMode = D3D11_CULL_NONE; break; }
	}
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	switch (info->fillmode) {
		case RG_RENDERPASS_FILLMODE_SOLID:     { rasterDesc.FillMode = D3D11_FILL_SOLID; break; }
		case RG_RENDERPASS_FILLMODE_WIREFRAME: { rasterDesc.FillMode = D3D11_FILL_WIREFRAME; break; }
		default: { rasterDesc.FillMode = D3D11_FILL_SOLID; break; }
	}
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	dev->dxdev->CreateRasterizerState(&rasterDesc, &rp->raster_state);


	if (info->dsv) {

#if R_DXRENDER_DEBUG
		if (info->dsv->type != R_DX_RESOURCEVIEW_DSV) {
			rgLogError(RG_LOG_RENDER, "DX11 Renderer: RResourceView->type(dst) must be a RG_RESOURCEVIEW_TYPE_DSV in RRenderpass creation!");
		}
#endif

		// Make depth-stencil state
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
		dev->dxdev->CreateDepthStencilState(&depthStencilDesc, &rp->depth_stencil_state);

		// TODO: check this resourceview
		rp->dsv = info->dsv->dsv;
	}

	return rp;
}

void R_DestroyRenderpass(RRenderpass* rp) {
	RRenderDevice* dev = rp->dev;
	rp->blend_state->Release();
	rp->raster_state->Release();
	if (rp->depth_stencil_state) {
		rp->depth_stencil_state->Release();
	}
	dev->allocator->Deallocate(rp);
}

RPipeline* R_CreatePipeline(RRenderDevice* dev, RPipelineCreateInfo* info) {
	return NULL;
}

void R_DestroyPipeline(RPipeline* pl) {
}

