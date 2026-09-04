#ifndef _RENDERTYPESDX_H
#define _RENDERTYPESDX_H

#include <rendertypes.h>
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
	Uint32               currentframe;
	Uint32               backbuffer_count;
	//ID3D11Texture2D*     backbuffers[8];

	RRenderpass*         default_renderpass;
	//RResourceView*       default_backbuffers[8];
	RFramebuffer*        default_framebuffers[8];

#if R_DXRENDER_DEBUG
	ID3D11Debug*         dxdbg;
	ID3D11InfoQueue*     dxdbginfoqueue;
#endif

	// Pipeline constant buffer
	RBuffer*             pushconstant;
	//RBuffer*             pc_pixel;

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
#define R_CMD_BIND_DESCRIPTOR    0x0014
#define R_CMD_BIND_SAMPLER       0x0015

#define R_CMD_UPDPUSHCONSTANTS   0x0016
#define R_CMD_PUSHCONSTANTS      0x0017

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
	RPipeline* pipeline;
	Uint32 commands_recorded;
	Uint32 max_commands;
};

#if 0

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
#endif

struct RDescriptorEntry {
	// Resource
	union {
		RImage* image;
		RBuffer* buffer;
		void* resource;
	};
	// View
	union {
		ID3D11ShaderResourceView* srv; // Texture / Structured buffer
		ID3D11UnorderedAccessView* uav; // Random access R/W buffers
		void* view_handle; // Dummy handle (Can be NULL for constant buffers)
	};
	Uint32 _offset1;
	Uint16 _offset2;
	Uint8 binding;
	Uint8 type; // RG_DESCRIPTOR_TYPE_
};

struct RDescriptorSet {
	RRenderDevice* dev;
	RDescriptorEntry entrys[16];
	Uint32 entry_count;
};

enum DXRM_STAGE {
	DXRM_STAGE_VERTEX   = 0x00,
	DXRM_STAGE_GEOMETRY = 0x01,
	DXRM_STAGE_PIXEL    = 0x02,
	DXRM_STAGE_COMPUTE  = 0x03,
	DXRM_STAGE_MAX      = 0x04
};
#define DX_RESOURCE_TYPE_PUSHCONSTANT_BINDING 0xFF // For runtime mapping

#define DX_RESOURCE_TYPE_PUSHCONSTANT   0x00 // b (raw buffer) max 255
#define DX_RESOURCE_TYPE_TEXTURE        0x01 // t (SRV)
#define DX_RESOURCE_TYPE_SAMPLER        0x02 // s (SAMPLER)
#define DX_RESOURCE_TYPE_CONSTANTBUFFER 0x03 // b (raw buffer)
#define DX_RESOURCE_TYPE_STORAGEBUFFER  0x04 // t/u SRV/UAV
#define DX_RESOURCE_TYPE_STORAGE_SRV    0x05 // t (SRV)
#define DX_RESOURCE_TYPE_STORAGE_UAV    0x06 // u (UAV)

struct DXRM {
	Uint8 valid;
	Uint8 type;
	Uint8 slot;
	Uint8 _padding;
};

struct DXResourceMapping {
	Uint16 idx; // HI - set, LOW - binding
	DXRM mappings[DXRM_STAGE_MAX];
};

struct RPipeline {
	RRenderDevice*        dev;
	Uint32 	              type;
	Uint32 	              bindings;
	DXResourceMapping*    map_table;
	ID3D11InputLayout*    layout;
	ID3D11VertexShader*   vs; // Vertex
	ID3D11PixelShader*    ps; // Pixel
	ID3D11GeometryShader* gs; // Geometry
	ID3D11ComputeShader*  cs; // Compute
	// Raster state
	ID3D11BlendState*        blend_state;
	ID3D11RasterizerState*   raster_state;
	ID3D11DepthStencilState* depth_stencil_state;
};

struct RFramebuffer {
	RRenderDevice* dev;
	Uint16         width;
	Uint16         height;
	Uint32         rtv_count;
	ID3D11RenderTargetView* rtv[6];  // Render target (dx11 only 8 RTVs)
	ID3D11DepthStencilView* dsv;     // Depth stencil
};

struct RRenderpass {
	RRenderDevice* dev;
	Uint32         rt_count;
	Uint32         use_depth;
	RRect          viewport;
};

struct MappingEntry {
	uint8_t set;     // Native Vulkan set
	uint8_t binding; // Native Vulkan binding
	uint8_t reg;     // Resource register
	uint8_t slot;    // Resource slot
};

struct RShader {
	RRenderDevice* dev;
	Uint8  type;
	Uint16 _padding;
	Uint16 mapping_count;
	Uint32 buffer_size;
	void* buffer;
	union {
		ID3D11VertexShader*   vs; // Vertex
		ID3D11PixelShader*    ps; // Pixel
		ID3D11GeometryShader* gs; // Geometry
		ID3D11ComputeShader*  cs; // Compute
	};
	MappingEntry* mapping_entrys;
};

struct RSampler {
	RRenderDevice* dev;
	ID3D11SamplerState* state;
};

static DXGI_FORMAT GetFormat(RFormat format) {
	switch (format) {
		case RG_FORMAT_R8_UNORM:           return DXGI_FORMAT_R8_UNORM;
		case RG_FORMAT_R8G8B8A8_UNORM:     return DXGI_FORMAT_R8G8B8A8_UNORM;
		case RG_FORMAT_R32_FLOAT:          return DXGI_FORMAT_R32_FLOAT;
		case RG_FORMAT_R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case RG_FORMAT_R32G32B32_FLOAT:    return DXGI_FORMAT_R32G32B32_FLOAT;
		case RG_FORMAT_R32G32_FLOAT:       return DXGI_FORMAT_R32G32_FLOAT;
		case RG_FORMAT_D24S8:              return DXGI_FORMAT_R24G8_TYPELESS;// DXGI_FORMAT_D24_UNORM_S8_UINT;
		case RG_FORMAT_D32:                return DXGI_FORMAT_R32_TYPELESS;// DXGI_FORMAT_D32_FLOAT;
		case RG_FORMAT_R16_FLOAT:          return DXGI_FORMAT_R16_FLOAT;
		case RG_FORMAT_R16G16_FLOAT:       return DXGI_FORMAT_R16G16_FLOAT;
		case RG_FORMAT_R16G16B16_FLOAT:    return DXGI_FORMAT_R16G16B16A16_FLOAT; // NO RGB 16F Format
		case RG_FORMAT_R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		default:                           return DXGI_FORMAT_UNKNOWN;
	}
}

static DXGI_FORMAT GetFormatView(RFormat format) {
	switch (format) {
		case RG_FORMAT_D24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case RG_FORMAT_D32:   return DXGI_FORMAT_D32_FLOAT;
		default:              return GetFormat(format);
	}
}

static Uint32 GetFormatSize(RFormat format) {
#if 0
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
#endif
	switch (format) {
		case RG_FORMAT_R8_UNORM:           return 1;
		case RG_FORMAT_R8G8B8A8_UNORM:     return 4;
		case RG_FORMAT_R32_FLOAT:          return 4;
		case RG_FORMAT_R32G32B32A32_FLOAT: return 16;
		case RG_FORMAT_R32G32B32_FLOAT:    return 12;
		case RG_FORMAT_R32G32_FLOAT:       return 8;
		case RG_FORMAT_D24S8:              return 4;
		case RG_FORMAT_D32:                return 4;
		case RG_FORMAT_R16_FLOAT:          return 2;
		case RG_FORMAT_R16G16_FLOAT:       return 4;
		case RG_FORMAT_R16G16B16_FLOAT:    return 8; // No RGB 16F
		case RG_FORMAT_R16G16B16A16_FLOAT: return 8;
		default:                           return 1;
	}
}

static D3D11_FILTER GetFilter(Uint8 filterMode) {
	switch (filterMode) {
		case RG_SAMPLER_FILTER_NEAREST:     return D3D11_FILTER_MIN_MAG_MIP_POINT;
		case RG_SAMPLER_FILTER_LINEAR:      return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		case RG_SAMPLER_FILTER_ANISOTROPIC: return D3D11_FILTER_ANISOTROPIC;
		default:                            return D3D11_FILTER_MIN_MAG_MIP_POINT;
	}
}

static D3D11_TEXTURE_ADDRESS_MODE GetAddressMode(Uint8 addressMode) {
	switch (addressMode) {
		case RG_SAMPLER_ADDRESSMODE_REPEAT:        return D3D11_TEXTURE_ADDRESS_WRAP;
		case RG_SAMPLER_ADDRESSMODE_MIRRORED:      return D3D11_TEXTURE_ADDRESS_MIRROR;
		case RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE: return D3D11_TEXTURE_ADDRESS_CLAMP;
		default:                                   return D3D11_TEXTURE_ADDRESS_WRAP;
	}
}

#endif