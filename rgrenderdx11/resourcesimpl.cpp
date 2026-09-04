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

	// Remove the RG_BUFFER_TYPE_VERTEX flag if the buffer is used as shader resource
	// its working perfectly on Nvidia and AMD drivers without this flag LoL

	if (RG_CHECK_FLAG(info->type, RG_BUFFER_TYPE_VERTEX) && (RG_CHECK_FLAG(info->type, RG_BUFFER_TYPE_UNORDERED) || RG_CHECK_FLAG(info->type, RG_BUFFER_TYPE_STRUCTURED))) {
		info->type &= ~RG_BUFFER_TYPE_VERTEX;
	}
	
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
#if R_DXRENDER_DEBUG
	if (result == E_INVALIDARG) {
		// Check for common mistakes
		if (buff.ByteWidth == 0) { rgLogError(RG_LOG_RENDER, "DX11 Renderer: Invalid ByteWidth passed (%d)", buff.ByteWidth); }
		if (buff.StructureByteStride == 0) { rgLogError(RG_LOG_RENDER, "DX11 Renderer: Invalid StructureByteStride passed (%d)", buff.StructureByteStride); }
		if ((buff.ByteWidth % buff.StructureByteStride) != 0) { rgLogError(RG_LOG_RENDER, "DX11 Renderer: Invalid buffer size or stride (%d %d)", buff.ByteWidth, buff.StructureByteStride); }
		if (buff.StructureByteStride > 2048) { rgLogError(RG_LOG_RENDER, "DX11 Renderer: Invalid ByteWidth passed (%d)", buff.ByteWidth); }
		if ((buff.BindFlags & D3D11_BIND_UNORDERED_ACCESS) == 0) { rgLogError(RG_LOG_RENDER, "DX11 Renderer: No RG_BUFFER_TYPE_UNORDERED flag passed!"); }
	}
#endif
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

void R_UpdateBuffer(RUpdateBufferInfo* info) {
	RRenderDevice* dev = info->handle->dev;
	if (info->handle->access == RG_BUFFER_ACCESS_GPU_ONLY) {
		// Ignore offset and length arguments
		dev->dxctx->UpdateSubresource(info->handle->buffer, 0, NULL, info->data, 0, 0);
	}
	else {
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT r = dev->dxctx->Map(info->handle->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		
		char* mdata = (char*)mappedResource.pData;
		SDL_memcpy(&mdata[info->offset], info->data, info->length);
		dev->dxctx->Unmap(info->handle->buffer, 0);
	}
}

//////////////////////////////////////////////////////////
// IMAGE
//////////////////////////////////////////////////////////

RImage* R_CreateImage(RRenderDevice* dev, RImageCreateInfo* info) {
	RImage* image = (RImage*)dev->allocator->Allocate(sizeof(RImage));
	image->dev    = dev;
	image->format = info->format;
	image->width  = info->width;
	image->height = info->height;

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width     = info->width;
	textureDesc.Height    = info->height;
	textureDesc.MipLevels = 0;
	textureDesc.ArraySize = 1;
	textureDesc.Format    = GetFormat(info->format);
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage     = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	if (info->format == RG_FORMAT_D24S8 || info->format == RG_FORMAT_D32) {
		textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		textureDesc.MipLevels = 1;
	} else {
		textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	textureDesc.CPUAccessFlags = 0;
	HRESULT r = dev->dxdev->CreateTexture2D(&textureDesc, NULL, &image->image);
	//if(r != )

	dev->imageMemLen += info->width * info->height * GetFormatSize(info->format);

	if (info->initialData) {
		// TODO: Support other formats and mip levels
		int rowPitch = info->width * GetFormatSize(info->format);
		dev->dxctx->UpdateSubresource(image->image, 0, NULL, info->initialData, rowPitch, 0);
	}

	return image;
}

void R_DestroyImage(RImage* image) {
	RRenderDevice* dev = image->dev;
	dev->imageMemLen -= image->width * image->height * GetFormatSize(image->format);
	image->image->Release();
	dev->allocator->Deallocate(image);
}

//////////////////////////////////////////////////////////
// RESOURCE VIEW
//////////////////////////////////////////////////////////
#if 0
RResourceView* R_CreateResourceView(RRenderDevice* dev, RResourceViewCreateInfo* info) {
	RResourceView* rv = (RResourceView*)dev->allocator->Allocate(sizeof(RResourceView));
	rv->dev = dev;
	
	// Render target view
	if (info->type == RG_RESOURCEVIEW_TYPE_RTV) {
		rv->type = R_DX_RESOURCEVIEW_RTV;

	}

	// Depth stencil view
	if (info->type == RG_RESOURCEVIEW_TYPE_DSV) {
		rv->type = R_DX_RESOURCEVIEW_DSV;

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format             = DXGI_FORMAT_D32_FLOAT;
		depthStencilViewDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		dev->dxdev->CreateDepthStencilView(info->dst_image->image, &depthStencilViewDesc, &rv->dsv);
	}

	// Shader resource view
	if (info->type == RG_RESOURCEVIEW_TYPE_SRV) {
		rv->type = R_DX_RESOURCEVIEW_SRV;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		if(info->buffer_type == RG_RESOURCEVIEW_IMAGE) {
			srvDesc.Format        = GetFormat(info->dst_image->format);
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels       = -1;
		}
		else {
			srvDesc.Format        = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

			srvDesc.BufferEx.FirstElement = 0;
			srvDesc.BufferEx.Flags        = 0;
			srvDesc.BufferEx.NumElements  = info->elements;
		}

		dev->dxdev->CreateShaderResourceView(info->dst_buffer->buffer, &srvDesc, &rv->srv);
		if (info->buffer_type == RG_RESOURCEVIEW_IMAGE && rv->srv) {
			dev->dxctx->GenerateMips(rv->srv);
		}
	}

	// Unordered access view
	if (info->type == RG_RESOURCEVIEW_TYPE_UAV) {
		rv->type = R_DX_RESOURCEVIEW_UAV;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		if (info->buffer_type == RG_RESOURCEVIEW_IMAGE) {
			uavDesc.Format        = GetFormat(info->dst_image->format);
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

			uavDesc.Texture2D.MipSlice = 0;
		}
		else {
			uavDesc.Format        = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements  = info->elements;
			uavDesc.Buffer.Flags        = 0;
		}
		dev->dxdev->CreateUnorderedAccessView(info->dst_buffer->buffer, &uavDesc, &rv->uav);
	}

	// Backbuffer view
	if (info->type == RG_RESOURCEVIEW_TYPE_BBV) {
		rv->type = R_DX_RESOURCEVIEW_RTV;

		ID3D11Texture2D* backbufferTex = NULL;
		//HRESULT t = dev->dxswapchain->GetBuffer(info->var, IID_ID3D11Texture2D, (void**)&backbufferTex);
		HRESULT t = dev->dxswapchain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backbufferTex);
		RG_ASSERT_MSG(SUCCEEDED(t), "Unable to get swapchain buffers");

		t = dev->dxdev->CreateRenderTargetView(backbufferTex, NULL, &rv->rtv);
		RG_ASSERT_MSG(SUCCEEDED(t), "Unable create backbuffer view");
		backbufferTex->Release();
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
	if (rv->type == R_DX_RESOURCEVIEW_SRV) {
		rv->srv->Release();
	}
	if (rv->type == R_DX_RESOURCEVIEW_UAV) {
		rv->uav->Release();
	}

	// Free rv

	dev->allocator->Deallocate(rv);
}
#endif

RFramebuffer* R_CreateFramebuffer(RRenderDevice* dev, RFramebufferCreateInfo* info) {
	RFramebuffer* fb = (RFramebuffer*)dev->allocator->Allocate(sizeof(RFramebuffer));
	fb->dev = dev;
	fb->width = info->width;
	fb->height = info->height;
	fb->rtv_count = info->rt_count;
	fb->dsv = NULL;

	// Make RTVs

	HRESULT t;
	for (Uint32 i = 0; i < fb->rtv_count; i++) {
		D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
		renderTargetViewDesc.Format = GetFormat(info->rts[i]->format);
		renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.Texture2D.MipSlice = 0;
		t = dev->dxdev->CreateRenderTargetView(info->rts[i]->image, &renderTargetViewDesc, &fb->rtv[i]);
		RG_ASSERT_MSG(SUCCEEDED(t), "Unable create backbuffer view");
		//fb->rtv[i] = info->rts[i]->image;
	}
	if (info->dsv) {
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format = GetFormatView(info->dsv->format);
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		t = dev->dxdev->CreateDepthStencilView(info->dsv->image, &depthStencilViewDesc, &fb->dsv);
		//RG_ASSERT_MSG(SUCCEEDED(t), "Unable create backbuffer view");
		//fb->dsv = info->dsv->dsv;
	}
	return fb;
}

void R_DestroyFramebuffer(RFramebuffer* fb) {
	RRenderDevice* dev = fb->dev;

	// Destroy RTVs
	for (Uint32 i = 0; i < fb->rtv_count; i++) {
		fb->rtv[i]->Release();
	}
	if (fb->dsv) {
		fb->dsv->Release();
	}

	dev->allocator->Deallocate(fb);
}

RSampler* R_CreateSampler(RRenderDevice* dev, RSamplerCreateInfo* info) {
	RSampler* sampler = (RSampler*)dev->allocator->Allocate(sizeof(RSampler));
	sampler->dev = dev;
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter         = GetFilter(info->filterMode);
	sampDesc.AddressU       = GetAddressMode(info->addressModeU);
	sampDesc.AddressV       = GetAddressMode(info->addressModeV);
	sampDesc.AddressW       = GetAddressMode(info->addressModeW);
	sampDesc.MipLODBias     = 0.0f;
	sampDesc.MaxAnisotropy  = info->maxAnisotropy;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampDesc.BorderColor[0] = 0;
	sampDesc.BorderColor[1] = 0;
	sampDesc.BorderColor[2] = 0;
	sampDesc.BorderColor[3] = 0;
	sampDesc.MinLOD         = 0;
	sampDesc.MaxLOD         = D3D11_FLOAT32_MAX;
	dev->dxdev->CreateSamplerState(&sampDesc, &sampler->state);
	return sampler;
}

void R_DestroySampler(RSampler* sampler) {
	RRenderDevice* dev = sampler->dev;
	sampler->state->Release();
	dev->allocator->Deallocate(sampler);
}

RDescriptorSet* R_CreateDescriptorSet(RRenderDevice* dev, RDescriptorSetCreateInfo* info) {
	RDescriptorSet* set = (RDescriptorSet*)dev->allocator->Allocate(sizeof(RDescriptorSet));
	set->dev = dev;
	set->entry_count = info->binding_count;

	// Make ResourceViews
	for (Uint32 i = 0; i < info->binding_count; i++) {
		RDescriptorSetBinding* binding = &info->bindings[i];

		HRESULT r = 0;
		set->entrys[i].binding = binding->binding;
		set->entrys[i].resource = binding->resource; // Just copy pointer
		set->entrys[i].type = binding->type;

		if (binding->type == RG_DESCRIPTOR_TYPE_IMAGE) {

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = GetFormat(binding->image->format);
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = -1;

			r = dev->dxdev->CreateShaderResourceView(binding->image->image, &srvDesc, &set->entrys[i].srv);
			if (binding->image->format != RG_FORMAT_D24S8 && binding->image->format != RG_FORMAT_D32) {
				dev->dxctx->GenerateMips(set->entrys[i].srv);
			}

		}
		else if (binding->type == RG_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
			if (RG_CHECK_FLAG(binding->buffer->type, RG_BUFFER_TYPE_UNORDERED)) {
				// Make UAV

				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				//if (info->buffer_type == RG_RESOURCEVIEW_IMAGE) {
				//	uavDesc.Format = GetFormat(info->dst_image->format);
				//	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				//	uavDesc.Texture2D.MipSlice = 0;
				//}
				//else {
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = binding->buffer->length;
				uavDesc.Buffer.Flags = 0;
				//}
				r = dev->dxdev->CreateUnorderedAccessView(binding->buffer->buffer, &uavDesc, &set->entrys[i].uav);

			} else {
				// Make SRV

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

				srvDesc.BufferEx.FirstElement = 0;
				srvDesc.BufferEx.Flags = 0;
				srvDesc.BufferEx.NumElements = binding->buffer->length;

				r = dev->dxdev->CreateShaderResourceView(binding->buffer->buffer, &srvDesc, &set->entrys[i].srv);
			}
		}

		rgLogInfo(RG_LOG_RENDER, "Error code: %d", r);

	}

	return set;
}

void R_DestroyDescriptorSet(RDescriptorSet* ds) {
	RRenderDevice* dev = ds->dev;

	for (Uint32 i = 0; i < ds->entry_count; i++) {
		RDescriptorEntry* entry = &ds->entrys[i];

		if (entry->type == RG_DESCRIPTOR_TYPE_IMAGE ||
			(entry->type == RG_DESCRIPTOR_TYPE_STORAGE_BUFFER && entry->buffer->type == RG_BUFFER_TYPE_STRUCTURED)) {
			if(ds->entrys[i].srv) ds->entrys[i].srv->Release();
		}
		else if (entry->type == RG_DESCRIPTOR_TYPE_STORAGE_BUFFER && entry->buffer->type == RG_BUFFER_TYPE_UNORDERED) {
			if (ds->entrys[i].uav) ds->entrys[i].uav->Release();
		}
	}

	dev->allocator->Deallocate(ds);
}