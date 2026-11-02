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
	//if (type & RG_BUFFER_TYPE_STRUCTURED) miscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	// SPIRV-Corss generates structured buffers as RAW buffers
	if (type & RG_BUFFER_TYPE_STRUCTURED) miscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
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
		//info->type &= ~RG_BUFFER_TYPE_VERTEX;
	}
	
	buffer->access = info->access;
	buffer->usage  = info->usage;
	buffer->type   = info->type;
	buffer->length = info->length;
	buffer->stride = info->stride;

	// Make buffer
	D3D11_BUFFER_DESC buff = {};
	buff.Usage = GetBufferUsage(buffer->usage);
	buff.ByteWidth = buffer->length;
	buff.BindFlags = GetBufferType(buffer->type);
	buff.CPUAccessFlags = GetBufferCPUAccess(buffer->access);
	buff.MiscFlags = GetBufferMiscFlags(buffer->type);
	buff.StructureByteStride = info->stride;

	// No structures, just raw buffer
	if (RG_CHECK_FLAG(buffer->type, RG_BUFFER_TYPE_STRUCTURED)) {
		buff.StructureByteStride = 0;
	}
	

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

static Uint64 GetImageSize(RImage* img) {
	Uint64 w = img->width;
	Uint64 h = img->height;
	Uint64 d = img->depth;
	if (img->type == RG_IMAGE_TYPE_TEXTURE_1D) {
		h = d = 1;
	}
	else if (img->type == RG_IMAGE_TYPE_TEXTURE_2D) {
		d = 1;
	}
	else if (img->type == RG_IMAGE_TYPE_TEXTURE_CUBE) {
		d = 6;
	}
	return w * h * d * GetFormatSize(img->format);
}

RImage* R_CreateImage(RRenderDevice* dev, RImageCreateInfo* info) {
	RImage* image = (RImage*)dev->allocator->Allocate(sizeof(RImage));
	image->dev    = dev;
	image->format = info->format;
	image->width  = info->width;
	image->height = info->height;
	image->depth  = info->depth;
	image->type   = info->type;
	if (info->type == RG_IMAGE_TYPE_TEXTURE_1D) {
		image->height = 1;
		image->depth  = 1;
	} else if (info->type == RG_IMAGE_TYPE_TEXTURE_2D || info->type == RG_IMAGE_TYPE_TEXTURE_CUBE) {
		image->depth = 1;
	}

	HRESULT r = 0;
	if (info->type == RG_IMAGE_TYPE_TEXTURE_1D) {
		D3D11_TEXTURE1D_DESC textureDesc1D = {};
		textureDesc1D.Width     = info->width;
		textureDesc1D.MipLevels = 1;
		textureDesc1D.ArraySize = 1;
		textureDesc1D.Format    = GetFormat(info->format);
		textureDesc1D.Usage     = D3D11_USAGE_DEFAULT;
		textureDesc1D.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc1D.CPUAccessFlags = 0;
		r = dev->dxdev->CreateTexture1D(&textureDesc1D, NULL, &image->image1d);
		image->height = 1;
		image->depth  = 1;
	}
	else if (info->type == RG_IMAGE_TYPE_TEXTURE_2D || info->type == RG_IMAGE_TYPE_TEXTURE_CUBE) {
		D3D11_TEXTURE2D_DESC textureDesc2D = {};
		textureDesc2D.Width     = info->width;
		textureDesc2D.Height    = info->height;
		textureDesc2D.MipLevels = 0;
		textureDesc2D.ArraySize = 1;
		textureDesc2D.Format    = GetFormat(info->format);
		textureDesc2D.SampleDesc.Count = 1;
		textureDesc2D.SampleDesc.Quality = 0;
		textureDesc2D.Usage     = D3D11_USAGE_DEFAULT;
		textureDesc2D.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc2D.CPUAccessFlags = 0;
		if (info->format == RG_FORMAT_D24S8 || info->format == RG_FORMAT_D32) {
			textureDesc2D.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
			textureDesc2D.MipLevels = 1;
		} else {
			textureDesc2D.BindFlags |= D3D11_BIND_RENDER_TARGET;
			textureDesc2D.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		if (info->type == RG_IMAGE_TYPE_TEXTURE_CUBE) {
			textureDesc2D.ArraySize = 6;
			textureDesc2D.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
		}

		r = dev->dxdev->CreateTexture2D(&textureDesc2D, NULL, &image->image2d);
		image->depth = 1;
	}
	else if (info->type == RG_IMAGE_TYPE_TEXTURE_3D) {
		D3D11_TEXTURE3D_DESC textureDesc3D = {};
		textureDesc3D.Width     = info->width;
		textureDesc3D.Height    = info->height;
		textureDesc3D.Depth     = info->depth;
		textureDesc3D.MipLevels = 0;
		textureDesc3D.Format    = GetFormat(info->format);
		textureDesc3D.Usage     = D3D11_USAGE_DEFAULT;
		textureDesc3D.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc3D.MiscFlags = 0;
		if (info->format == RG_FORMAT_D24S8 || info->format == RG_FORMAT_D32) {
			textureDesc3D.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
			textureDesc3D.MipLevels = 1;
		} else {
			textureDesc3D.BindFlags |= D3D11_BIND_RENDER_TARGET;
			textureDesc3D.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}
		textureDesc3D.CPUAccessFlags = 0;
		r = dev->dxdev->CreateTexture3D(&textureDesc3D, NULL, &image->image3d);
	}

	if (FAILED(r)) {
		dev->allocator->Deallocate(image);
		return NULL;
	}

	dev->imageMemLen += GetImageSize(image);

	if (info->initialData) {
		// TODO: Support other formats and mip levels
		int rowPitch = info->width * GetFormatSize(info->format);
		dev->dxctx->UpdateSubresource(image->image, 0, NULL, info->initialData, rowPitch, 0);
	}

	return image;
}

void R_DestroyImage(RImage* image) {
	RRenderDevice* dev = image->dev;
	dev->imageMemLen -= GetImageSize(image);

	if (image->type == RG_IMAGE_TYPE_TEXTURE_1D) {
		image->image1d->Release();
	} else if (image->type == RG_IMAGE_TYPE_TEXTURE_2D) {
		image->image2d->Release();
	} else if (image->type == RG_IMAGE_TYPE_TEXTURE_3D) {
		image->image3d->Release();
	} else if (image->type == RG_IMAGE_TYPE_TEXTURE_CUBE) {
		image->image2d->Release();
	}
	
	dev->allocator->Deallocate(image);
}

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
		renderTargetViewDesc.Format = GetFormatRT(info->rts[i]->format);
		if (info->rts[i]->type == RG_IMAGE_TYPE_TEXTURE_2D) {
			renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			renderTargetViewDesc.Texture2D.MipSlice = 0;
		} else if (info->rts[i]->type == RG_IMAGE_TYPE_TEXTURE_3D) {
			renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
			renderTargetViewDesc.Texture3D.MipSlice = 0;
		} else if (info->rts[i]->type == RG_IMAGE_TYPE_TEXTURE_CUBE) {
			renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			renderTargetViewDesc.Texture2DArray.FirstArraySlice = 0;
			renderTargetViewDesc.Texture2DArray.MipSlice = 0;
			renderTargetViewDesc.Texture2DArray.ArraySize = 6;
		}

		t = dev->dxdev->CreateRenderTargetView(info->rts[i]->image, &renderTargetViewDesc, &fb->rtv[i]);
		RG_ASSERT_MSG(SUCCEEDED(t), "Unable create backbuffer view");
		//fb->rtv[i] = info->rts[i]->image;
	}
	if (info->dsv) {
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format = GetFormatRT(info->dsv->format);
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		if (info->dsv->type == RG_IMAGE_TYPE_TEXTURE_2D) {
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;
		} else if (info->dsv->type == RG_IMAGE_TYPE_TEXTURE_CUBE) {
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			depthStencilViewDesc.Texture2DArray.FirstArraySlice = 0;
			depthStencilViewDesc.Texture2DArray.MipSlice = 0;
			depthStencilViewDesc.Texture2DArray.ArraySize = 6;
		}
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
			srvDesc.Format = GetFormatSRV(binding->image->format);
			if (binding->image->type == RG_IMAGE_TYPE_TEXTURE_1D) {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
				srvDesc.Texture1D.MostDetailedMip = 0;
				srvDesc.Texture1D.MipLevels = -1;
			} else if (binding->image->type == RG_IMAGE_TYPE_TEXTURE_2D) {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = -1;
			} else if (binding->image->type == RG_IMAGE_TYPE_TEXTURE_3D) {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
				srvDesc.Texture3D.MostDetailedMip = 0;
				srvDesc.Texture3D.MipLevels = -1;
			} else if (binding->image->type == RG_IMAGE_TYPE_TEXTURE_CUBE) {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip = 0;
				srvDesc.TextureCube.MipLevels = -1;
			}

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
				uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = binding->buffer->length / 4;// binding->buffer->stride;
				uavDesc.Buffer.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
				//}
				r = dev->dxdev->CreateUnorderedAccessView(binding->buffer->buffer, &uavDesc, &set->entrys[i].uav);

			} else {
				// Make SRV

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

				srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;

				srvDesc.BufferEx.FirstElement = 0;
				srvDesc.BufferEx.NumElements = binding->buffer->length / 4;// binding->buffer->stride;
				srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;

				r = dev->dxdev->CreateShaderResourceView(binding->buffer->buffer, &srvDesc, &set->entrys[i].srv);
			}
		}

		if (r != S_OK) {
			__debugbreak();
		}

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