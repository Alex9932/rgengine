#ifndef _RENDERTYPESDX_H
#define _RENDERTYPESDX_H

#include "rendertypes.h"
#include <d3d11.h>
#include <allocator.h>

struct RRenderDevice {
	ID3D11Device*        dxdev;
	ID3D11DeviceContext* dxctx;
	IDXGISwapChain*      dxswapchain;

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
	RRenderDevice* dev;

};

struct RCommandBuffer {
	RRenderDevice* dev;
	Engine::PoolAllocator* pool;
};

#endif