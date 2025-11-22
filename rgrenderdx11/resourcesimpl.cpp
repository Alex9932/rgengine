#include <rshared.h>
#include "rendertypesdx.h"
#include <engine.h>

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
	buff.Usage = GetBufferUsage(buffer->usage);
	buff.ByteWidth = buffer->length;
	buff.BindFlags = GetBufferType(buffer->type);
	buff.CPUAccessFlags = GetBufferCPUAccess(buffer->access);
	buff.MiscFlags = GetBufferMiscFlags(buffer->type);
	buff.StructureByteStride = info->stride;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = info->initialData;
	data.SysMemPitch = 1;
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

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = info->width;
	textureDesc.Height = info->height;
	textureDesc.MipLevels = 0;
	textureDesc.ArraySize = 1;
	textureDesc.Format = GetFormat(info->format);
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	if (info->format == RG_FORMAT_D24S8 || info->format == RG_FORMAT_D32) {
		textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	} else {
		textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	textureDesc.CPUAccessFlags = 0;
	dev->dxdev->CreateTexture2D(&textureDesc, NULL, &image->image);

	return image;
}

void R_DestroyImage(RImage* image) {
	RRenderDevice* dev = image->dev;
	image->image->Release();
	dev->allocator->Deallocate(image);
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
#if 0
	if (info->type == RG_RESOURCEVIEW_TYPE_SRV) {
		rv->type = R_DX_RESOURCEVIEW_SRV;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = info->dst_image->format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		dev->dxdev->CreateShaderResourceView(this->texture, &srvDesc, &rv->srv);
		dev->dxctx->GenerateMips(this->shaderResource);
	}
#endif
	if (info->type == RG_RESOURCEVIEW_TYPE_DSV) {
		rv->type = R_DX_RESOURCEVIEW_DSV;
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		dev->dxdev->CreateDepthStencilView(info->dst_image->image, &depthStencilViewDesc, &rv->dsv);
	}

	return rv;
}

void R_DestroyResourceView(RResourceView* rv) {
	RRenderDevice* dev = rv->dev;

	if (rv->type == R_DX_RESOURCEVIEW_RTV) {
		rv->rtv->Release();
	}
	if (rv->type == R_DX_RESOURCEVIEW_DSV) {
		rv->dsv->Release();
	}

	// Free rv

	dev->allocator->Deallocate(rv);
}