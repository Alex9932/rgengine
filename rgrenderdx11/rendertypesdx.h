#ifndef _RENDERTYPESDX_H
#define _RENDERTYPESDX_H

#include "rendertypes.h"
#include <d3d11.h>
#include <allocator.h>

#define R_RENDERER_NAME      "DirectX 11"
#define R_RENDERER_SHORTNAME "dx11"

#define R_DXRENDER_DEBUG 1

#define R_MAX_COMMANDS_PER_BUFFER 256

struct RRenderDevice {
	ID3D11Device*        dxdev;
	ID3D11DeviceContext* dxctx;
	IDXGISwapChain*      dxswapchain;
	ID3D11Texture2D*     backbuffers[8];

#if R_DXRENDER_DEBUG
	ID3D11Debug*         dxdbg;
	ID3D11InfoQueue*     dxdbginfoqueue;
#endif

	// Pipeline constant buffers
	RBuffer*             pc_vertex;
	RBuffer*             pc_pixel;

	SDL_Window*          hwnd;
	ivec2                wndsize;
	Bool 			     wndresized;

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
	RFormat		     format;
	Uint16		     width;
	Uint16		     height;
};

#define R_CMD_NOP                0x0000

#define R_CMD_BEGIN_RENDERPASS   0x0001
#define R_CMD_END_RENDERPASS     0x0002
#define R_CMD_CLEAR_RT           0x0003

#define R_CMD_BIND_PIPELINE      0x0011
#define R_CMD_BIND_VERTEX_BUFFER 0x0012
#define R_CMD_BIND_INDEX_BUFFER  0x0013
#define R_CMD_BIND_RESOURCEVIEWS 0x0014

#define R_CMD_PUSHCONSTANTS      0x0015

#define R_CMD_DRAW_IMGUI         0x0021
#define R_CMD_DRAW               0x0022
#define R_CMD_DRAW_INDEXED       0x0023

#define R_CMD_DISPATCH           0x0031

struct RCommand {
	void*  handle;
	Uint16 cmd;
	Uint16 _off0;
	Uint32 _off1;
	union {
		struct {
			Uint32 data0; Uint32 data1; Uint32 data2;
			Uint32 data3; Uint32 data4; Uint32 data5;
		};
		Uint32 data[32];
		char buffer[128]; // for push constants
	};
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
	RRenderDevice*        dev;
	Uint32 	              type;
	Uint32 	              _off0;
	ID3D11InputLayout*    layout;
	ID3D11VertexShader*   vs; // Vertex
	ID3D11PixelShader*    ps; // Pixel
	ID3D11GeometryShader* gs; // Geometry
	ID3D11ComputeShader*  cs; // Compute
};

struct RRenderpass {
	RRenderDevice*           dev;
	Uint32 				     rtv_count;
	RRect                    viewport;
	ID3D11RenderTargetView*  rtv[6];  // Render target (dx11 only 8 RTVs)
	ID3D11DepthStencilView*  dsv;     // Depth stencil
	ID3D11BlendState*        blend_state;
	ID3D11RasterizerState*   raster_state;
	ID3D11DepthStencilState* depth_stencil_state;
};

struct RShader {
	RRenderDevice* dev;
	Uint8  type;
	Bool   isCompiled;
	Uint16 _offset1;
	Uint32 _offset2;
	ID3D10Blob* buffer;
	union {
		ID3D11VertexShader*   vs; // Vertex
		ID3D11PixelShader*    ps; // Pixel
		ID3D11GeometryShader* gs; // Geometry
		ID3D11ComputeShader*  cs; // Compute
	};
};

static DXGI_FORMAT GetFormat(RFormat format) {
	switch (format) {
		//case RG_FORMAT_UNKNOWN:         return DXGI_FORMAT_UNKNOWN;
		case RG_FORMAT_R8_UNORM:        return DXGI_FORMAT_R8_UNORM;
		case RG_FORMAT_R8G8B8A8_UNORM:  return DXGI_FORMAT_R8G8B8A8_UNORM;
		case RG_FORMAT_R32_FLOAT:       return DXGI_FORMAT_R32_FLOAT;
		case RG_TEXTURE_F32_RGBA:       return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case RG_FORMAT_D24S8:           return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case RG_FORMAT_R32G32_FLOAT:    return DXGI_FORMAT_R32G32_FLOAT;
		case RG_FORMAT_R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
		case RG_FORMAT_D32:             return DXGI_FORMAT_R32_TYPELESS;
		default: return DXGI_FORMAT_UNKNOWN;
	}
}

static Uint32 GetFormatSize(RFormat format) {
	switch (format) {
		//case RG_FORMAT_UNKNOWN:         return DXGI_FORMAT_UNKNOWN;
		case RG_FORMAT_R8_UNORM:        return 1;
		case RG_FORMAT_R8G8B8A8_UNORM:  return 4;
		case RG_FORMAT_R32_FLOAT:       return 4;
		case RG_TEXTURE_F32_RGBA:       return 16;
		case RG_FORMAT_D24S8:           return 4;
		case RG_FORMAT_R32G32_FLOAT:    return 16;
		case RG_FORMAT_R32G32B32_FLOAT: return 32;
		case RG_FORMAT_D32:             return 4;
		default: return 1;
	}
}

#endif