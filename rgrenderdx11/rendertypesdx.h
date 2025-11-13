#ifndef _RENDERTYPESDX_H
#define _RENDERTYPESDX_H

#include "rendertypes.h"
#include <d3d11.h>
#include <allocator.h>

#define R_MAX_COMMANDS_PER_BUFFER 256

struct RRenderDevice {
	ID3D11Device*        dxdev;
	ID3D11DeviceContext* dxctx;
	IDXGISwapChain*      dxswapchain;
	ID3D11Texture2D*     backbuffers[8];

	Engine::Allocator* allocator;

	Uint64 buffersMemLen;
	Uint64 imageMemLen;
	Uint32 flags;

	char cardName[128];
};

struct RBuffer {
	RRenderDevice* dev;
	ID3D11Buffer*  buffer;
	Uint64         length;
	Uint8          access;
	Uint8          usage;
	Uint16         type;
	Uint32 _offset2;
};

struct RImage {
	RRenderDevice*   dev;
	ID3D11Texture2D* image;
};

#define R_CMD_NOP                0x0000

#define R_CMD_BEGIN_RENDERPASS   0x0001
#define R_CMD_END_RENDERPASS     0x0002

#define R_CMD_BIND_PIPELINE      0x0011
#define R_CMD_BIND_VERTEX_BUFFER 0x0012
#define R_CMD_BIND_INDEX_BUFFER  0x0013

#define R_CMD_DRAW_IMGUI         0x0021
#define R_CMD_DRAW               0x0022
#define R_CMD_DRAW_INDEXED       0x0023

struct RCommand {
	void*  handle;
	Uint16 cmd;
	Uint16 _offset0;
	Uint32 data0;
	Uint32 data1;
	Uint32 data2;
	Uint32 data3;
	Uint32 data4;
};

struct RCommandBuffer {
	RRenderDevice* dev;
	Engine::LinearAllocator* pool;
	Uint32 commands_recorded;
};

#define R_DX_RESOURCEVIEW_RTV 0x1
#define R_DX_RESOURCEVIEW_DSV 0x2
#define R_DX_RESOURCEVIEW_SRV 0x3
#define R_DX_RESOURCEVIEW_UAV 0x4

struct RResourceView {
	RRenderDevice* dev;
	union {
		ID3D11RenderTargetView*    rtv;
		ID3D11DepthStencilView*    dsv;
		ID3D11ShaderResourceView*  srv;
		ID3D11UnorderedAccessView* uav;
	};
	Uint8 type;
	// Padding offsets
	Uint8 _offset0;
	Uint16 _offset1;
	Uint32 _offset2;
};

struct RPipeline {
	RRenderDevice* dev;
	
};

struct RRenderpass {
	RRenderDevice*           dev;
	Uint32 				     rtv_count;
	Uint32 				     _offset; // For memory alignment
	ID3D11RenderTargetView*  rtv[10]; // Render target (dx11 only 8 RTVs)
	ID3D11DepthStencilView*  dsv;     // Depth stencil
	ID3D11BlendState*        blend_state;
	ID3D11RasterizerState*   raster_state;
	ID3D11DepthStencilState* depth_stencil_state;
};

#endif