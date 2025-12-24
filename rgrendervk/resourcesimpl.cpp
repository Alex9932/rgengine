#include <rshared.h>
#include <vma/vk_mem_alloc.h>
#include "rendertypesvk.h"

static VkBufferUsageFlags GetBufferType(Uint16 type) {
	VkBufferUsageFlags usage = 0;
	if (type & RG_BUFFER_TYPE_VERTEX)     usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (type & RG_BUFFER_TYPE_INDEX)      usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (type & RG_BUFFER_TYPE_CONSTANT)   usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (type & RG_BUFFER_TYPE_SHADER_RES) usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (type & RG_BUFFER_TYPE_UNORDERED)  usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (type & RG_BUFFER_TYPE_VK_TSRC)    usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (type & RG_BUFFER_TYPE_VK_TDST)    usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	return usage;
}

static VmaAllocationCreateFlagBits GetFlags(Uint32 access) {
	Uint32 flags = 0;
	if (access == RG_BUFFER_ACCESS_GPU_ONLY) {
		//flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	}
	if (access == RG_BUFFER_ACCESS_CPU_WRITE) {
		flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	}
	if (access == RG_BUFFER_ACCESS_CPU_READ) {
		flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
	}
	return (VmaAllocationCreateFlagBits)flags;
}

static VmaMemoryUsage GetUsage(Uint8 usage, Uint8 access) {
	if (usage == RG_BUFFER_USAGE_DEFAULT && access == RG_BUFFER_ACCESS_GPU_ONLY) {
		return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	}
	if (usage == RG_BUFFER_USAGE_DYNAMIC && access == RG_BUFFER_ACCESS_CPU_WRITE) {
		return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	}
	return VMA_MEMORY_USAGE_AUTO;
}

static void CopyToImage(RBuffer* src, RImage* dst, RImageCreateInfo* info) {
	RRenderDevice* dev = src->dev;
	VkCommandBuffer cmdbuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = dev->vkcommandpool;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(dev->vkdev, &allocInfo, &cmdbuffer);
	vkResetCommandBuffer(cmdbuffer, 0);

	VkCommandBufferBeginInfo begininfo = {};
	begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdbuffer, &begininfo);

	// Transition image layout to TRANSFER_DST_OPTIMAL
	VkImageMemoryBarrier barrier = {};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask       = 0;
	barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = dst->image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;
	vkCmdPipelineBarrier(cmdbuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &barrier);

	// Copy buffer to image
	VkBufferImageCopy region = {};
	region.bufferOffset                    = 0;
	region.bufferRowLength                 = 0;
	region.bufferImageHeight               = 0;
	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = 1;
	region.imageOffset.x                   = 0;
	region.imageOffset.y                   = 0;
	region.imageOffset.z                   = 0;
	region.imageExtent.width               = info->width;
	region.imageExtent.height              = info->height;
	region.imageExtent.depth               = 1;
	vkCmdCopyBufferToImage(cmdbuffer, src->buffer, dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// Transition image layout to SHADER_READ_ONLY_OPTIMAL
	barrier = {};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = dst->image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;
	vkCmdPipelineBarrier(cmdbuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &barrier);

	vkEndCommandBuffer(cmdbuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdbuffer;
	vkQueueSubmit(dev->vkqueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(dev->vkqueue);

	vkFreeCommandBuffers(dev->vkdev, dev->vkcommandpool, 1, &cmdbuffer);
}

static void CopyToBuffer(RBuffer* src, RBuffer* dst, RBufferCreateInfo* info) {
	RRenderDevice* dev = src->dev;
	VkCommandBuffer cmdbuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = dev->vkcommandpool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(dev->vkdev, &allocInfo, &cmdbuffer);
	vkResetCommandBuffer(cmdbuffer, 0);

	VkCommandBufferBeginInfo begininfo = {};
	begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdbuffer, &begininfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size      = info->length;
	vkCmdCopyBuffer(cmdbuffer, src->buffer, dst->buffer, 1, &copyRegion);

	vkEndCommandBuffer(cmdbuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdbuffer;
	vkQueueSubmit(dev->vkqueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(dev->vkqueue);
	vkFreeCommandBuffers(dev->vkdev, dev->vkcommandpool, 1, &cmdbuffer);
}

RBuffer* R_CreateBuffer(RRenderDevice* dev, RBufferCreateInfo* info) {
	RBuffer* buffer = (RBuffer*)dev->allocator->Allocate(sizeof(RBuffer));
	buffer->dev = dev;
	buffer->length = info->length;
	buffer->access = info->access;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size        = info->length;
	bufferInfo.usage       = GetBufferType(info->type);
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (info->initialData && info->access == RG_BUFFER_ACCESS_GPU_ONLY) {
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}


	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.flags = GetFlags(info->access);
	allocCreateInfo.usage = GetUsage(info->usage, info->access);

	vmaCreateBuffer(dev->vmaallocator, &bufferInfo, &allocCreateInfo, &buffer->buffer, &buffer->allocation, NULL);

	if (info->initialData) {
		if (info->access != RG_BUFFER_ACCESS_GPU_ONLY) {
			// For CPU_WRITE buffers
			RUpdateBufferInfo updateInfo = {};
			updateInfo.handle = buffer;
			updateInfo.offset = 0;
			updateInfo.length = info->length;
			updateInfo.data   = info->initialData;
			R_UpdateBuffer(&updateInfo);
		} else {
			// For GPU_ONLY buffers
			RBufferCreateInfo stagingInfo = {};
			stagingInfo.length = info->length;
			stagingInfo.type   = RG_BUFFER_TYPE_VK_TSRC;
			stagingInfo.usage  = RG_BUFFER_USAGE_DYNAMIC;
			stagingInfo.access = RG_BUFFER_ACCESS_CPU_WRITE;
			stagingInfo.initialData = info->initialData;
			RBuffer* stagingBuffer = R_CreateBuffer(dev, &stagingInfo);
			CopyToBuffer(stagingBuffer, buffer, info);
			R_DestroyBuffer(stagingBuffer);
		}
	}

	dev->buffersMemLen += buffer->length;

	return buffer;
}

void R_DestroyBuffer(RBuffer* buffer) {
	RRenderDevice* dev = buffer->dev;
	vmaDestroyBuffer(dev->vmaallocator, buffer->buffer, buffer->allocation);
	dev->buffersMemLen -= buffer->length;
	dev->allocator->Deallocate(buffer);
}

void R_UpdateBuffer(RUpdateBufferInfo* info) {
	RRenderDevice* dev = info->handle->dev;

	if (info->handle->access == RG_BUFFER_ACCESS_GPU_ONLY) {
		// TODO: Make staging buffer and copy data
		return;
	}

	void* data;
	VkResult res = vmaMapMemory(dev->vmaallocator, info->handle->allocation, &data);
	if (res == VK_SUCCESS) {
		char* dst = (char*)data;
		SDL_memcpy(&dst[info->offset], info->data, info->length);
		vmaFlushAllocation(dev->vmaallocator, info->handle->allocation, info->offset, info->length);
		vmaUnmapMemory(dev->vmaallocator, info->handle->allocation);
	}
}

RImage* R_CreateImage(RRenderDevice* dev, RImageCreateInfo* info) {
	RImage* image = (RImage*)dev->allocator->Allocate(sizeof(RImage));
	image->dev = dev;
	image->length = info->width * info->height * GetImageFormatSize(info->format);
	image->format = info->format;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.format        = GetImageFormat(info->format);
	imageInfo.extent.width  = info->width;
	imageInfo.extent.height = info->height;
	imageInfo.extent.depth  = 1;
	imageInfo.mipLevels     = 1;
	imageInfo.arrayLayers   = 1;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (info->format == RG_FORMAT_D24S8 || info->format == RG_FORMAT_D32) {
		imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else {
		imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	vmaCreateImage(dev->vmaallocator, &imageInfo, &allocCreateInfo, &image->image, &image->allocation, NULL);

	// Upload initial data if provided
	if (info->initialData) {
		RBufferCreateInfo stagingInfo = {};
		stagingInfo.length = image->length;
		stagingInfo.type   = RG_BUFFER_TYPE_VK_TSRC;
		stagingInfo.usage  = RG_BUFFER_USAGE_DYNAMIC;
		stagingInfo.access = RG_BUFFER_ACCESS_CPU_WRITE;
		stagingInfo.initialData = info->initialData;
		RBuffer* stagingBuffer = R_CreateBuffer(dev, &stagingInfo);
		CopyToImage(stagingBuffer, image, info);
		R_DestroyBuffer(stagingBuffer);
	}

	dev->buffersMemLen += image->length;

	return image;
}

void R_DestroyImage(RImage* image) {
	RRenderDevice* dev = image->dev;
	vmaDestroyImage(dev->vmaallocator, image->image, image->allocation);
	dev->imageMemLen -= image->length;
	dev->allocator->Deallocate(image);
}

RFramebuffer* R_CreateFramebuffer(RRenderDevice* dev, RFramebufferCreateInfo* info) {
	RFramebuffer* fb = (RFramebuffer*)dev->allocator->Allocate(sizeof(RFramebuffer));
	fb->dev    = dev;
	fb->width  = info->width;
	fb->height = info->height;

	VkImageView attachments[8];
	for (Uint32 i = 0; i < info->rt_count; i++) {
		attachments[i] = info->rts[i]->imageView;
	}

	if (info->dsv) {
		attachments[info->rt_count] = info->dsv->imageView;
	}

	VkFramebufferCreateInfo fbInfo = {};
	fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.renderPass      = info->renderpass->renderpass;
	fbInfo.attachmentCount = info->rt_count;
	if (info->dsv) {
		fbInfo.attachmentCount++;
	}
	fbInfo.pAttachments    = attachments;
	fbInfo.width           = info->width;
	fbInfo.height          = info->height;
	fbInfo.layers          = 1;
	vkCreateFramebuffer(dev->vkdev, &fbInfo, dev->vkalloc, &fb->framebuffer);

	return fb;
}

void R_DestroyFramebuffer(RFramebuffer* fb) {
	RRenderDevice* dev = fb->dev;
	vkDestroyFramebuffer(dev->vkdev, fb->framebuffer, dev->vkalloc);
	dev->allocator->Deallocate(fb);
}

RCommandBuffer* R_CreateCommandBuffer(RRenderDevice* dev, RCommandBufferCreateInfo* info) {
	RCommandBuffer* buffer = (RCommandBuffer*)dev->allocator->Allocate(sizeof(RCommandBuffer));
	buffer->dev = dev;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = dev->vkcommandpool;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(dev->vkdev, &allocInfo, &buffer->cmdbuffer);

	return buffer;
}

void R_DestroyCommandBuffer(RCommandBuffer* buffer) {
	RRenderDevice* dev = buffer->dev;
	vkFreeCommandBuffers(dev->vkdev, dev->vkcommandpool, 1, &buffer->cmdbuffer);
	dev->allocator->Deallocate(buffer);
}

void R_ResetCommandBuffer(RCommandBuffer* buffer) {
	vkResetCommandBuffer(buffer->cmdbuffer, 0);
}

void R_BeginCommandBuffer(RCommandBuffer* buffer) {
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(buffer->cmdbuffer, &info);
}

void R_EndCommandBuffer(RCommandBuffer* buffer) {
	vkEndCommandBuffer(buffer->cmdbuffer);
}

void R_SubmitCommandBuffer(RCommandBufferSubmitInfo* info) {
	// TODO: Support semaphores and fences
	RRenderDevice* dev = info->buffer->dev;
	VkSubmitInfo submitInfo = {};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &info->buffer->cmdbuffer;
	vkQueueSubmit(dev->vkqueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(dev->vkqueue);
}

RResourceView* R_CreateResourceView(RRenderDevice* dev, RResourceViewCreateInfo* info) {
	RResourceView* rv = (RResourceView*)dev->allocator->Allocate(sizeof(RResourceView));
	rv->dev = dev;
	rv->buffer_type = info->buffer_type;

	VkDescriptorSetLayoutBinding binding = {};

	binding.binding         = 0;
	binding.descriptorCount = 1;
	//binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding.stageFlags      = GetShaderStage(info->stage);

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &binding;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	if (info->buffer_type == RG_RESOURCEVIEW_IMAGE) {
		VkImage image = NULL;
		RFormat format = RG_FORMAT_UNKNOWN;
		VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;

		if (info->type == RG_RESOURCEVIEW_TYPE_BBV) {
			image  = dev->vkswapimages[info->var];
			format = RG_FORMAT_R8G8B8A8_UNORM;
		} else {
			image  = info->dst_image->image;
			format = info->dst_image->format;
#if 0
			if (format == RG_FORMAT_D24S8) {
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			if (format == RG_FORMAT_D32) {
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
#else
			if (format == RG_FORMAT_D24S8 || format == RG_FORMAT_D32) {
				aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
#endif
		}

		rv->format = format;

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image                           = image;
		viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format                          = GetImageFormat(format);
		viewInfo.subresourceRange.aspectMask     = aspect;
		viewInfo.subresourceRange.baseMipLevel   = 0;
		viewInfo.subresourceRange.levelCount     = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount     = 1;
		vkCreateImageView(dev->vkdev, &viewInfo, dev->vkalloc, &rv->imageView);

		rv->descLayout = NULL;
		rv->descSet = NULL;

		if (info->type != RG_RESOURCEVIEW_TYPE_BBV) {
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			vkCreateDescriptorSetLayout(dev->vkdev, &layoutInfo, dev->vkalloc, &rv->descLayout);

			allocInfo.descriptorPool = dev->vkdescriptorpool[2];
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &rv->descLayout;
			vkAllocateDescriptorSets(dev->vkdev, &allocInfo, &rv->descSet);


			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageView = rv->imageView;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = rv->descSet;
			write.dstBinding = 0;
			write.dstArrayElement = 0;
			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			write.descriptorCount = 1;
			write.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(dev->vkdev, 1, &write, 0, nullptr);
		}
	} else {

		rv->format = RG_FORMAT_R8_UNORM;

		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vkCreateDescriptorSetLayout(dev->vkdev, &layoutInfo, dev->vkalloc, &rv->descLayout);

		allocInfo.descriptorPool = dev->vkdescriptorpool[0];
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &rv->descLayout;
		vkAllocateDescriptorSets(dev->vkdev, &allocInfo, &rv->descSet);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = info->dst_buffer->buffer;
		bufferInfo.offset = 0;
		bufferInfo.range  = VK_WHOLE_SIZE;

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = rv->descSet;
		write.dstBinding = 0;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(dev->vkdev, 1, &write, 0, nullptr);
	}



	return rv;
}

void R_DestroyResourceView(RResourceView* rv) {
	RRenderDevice* dev = rv->dev;

	if (rv->buffer_type == RG_RESOURCEVIEW_IMAGE) {
		if (rv->descSet) vkFreeDescriptorSets(dev->vkdev, dev->vkdescriptorpool[2], 1, &rv->descSet);
		vkDestroyImageView(dev->vkdev, rv->imageView, dev->vkalloc);
	} else {
		vkFreeDescriptorSets(dev->vkdev, dev->vkdescriptorpool[0], 1, &rv->descSet);
	}

	if (rv->descSet) vkDestroyDescriptorSetLayout(dev->vkdev, rv->descLayout, dev->vkalloc);

	dev->allocator->Deallocate(rv);
}

static VkFilter GetSamplerFilter(Uint8 filterMode) {
	if (filterMode == RG_SAMPLER_FILTER_NEAREST) {
		return VK_FILTER_NEAREST;
	}
	return VK_FILTER_LINEAR;
}

static VkSamplerAddressMode GetSamplerAddressMode(Uint8 addressMode) {
	switch (addressMode) {
		case RG_SAMPLER_ADDRESSMODE_REPEAT:        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case RG_SAMPLER_ADDRESSMODE_MIRRORED:      return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		default:                                   return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

RSampler* R_CreateSampler(RRenderDevice* dev, RSamplerCreateInfo* info) {
	RSampler* sampler = (RSampler*)dev->allocator->Allocate(sizeof(RSampler));
	sampler->dev = dev;
	VkSamplerCreateInfo sampInfo = {};
	sampInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampInfo.magFilter        = GetSamplerFilter(info->filterMode);
	sampInfo.minFilter        = GetSamplerFilter(info->filterMode);
	sampInfo.addressModeU     = GetSamplerAddressMode(info->addressModeU);
	sampInfo.addressModeV     = GetSamplerAddressMode(info->addressModeV);
	sampInfo.addressModeW     = GetSamplerAddressMode(info->addressModeW);
	sampInfo.anisotropyEnable = (info->filterMode == RG_SAMPLER_FILTER_ANISOTROPIC) ? VK_TRUE : VK_FALSE;
	sampInfo.maxAnisotropy    = info->maxAnisotropy;
	sampInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	//sampInfo.unnormalizedCoordinates = VK_FALSE;
	sampInfo.compareEnable    = VK_FALSE;
	sampInfo.compareOp        = VK_COMPARE_OP_ALWAYS;
	sampInfo.mipLodBias       = 0.0f;
	sampInfo.minLod           = 0.0f;
	sampInfo.maxLod           = 0.0f;
	vkCreateSampler(dev->vkdev, &sampInfo, dev->vkalloc, &sampler->sampler);

	VkDescriptorSetLayoutBinding binding = {};
	binding.binding         = 0;
	binding.descriptorCount = 1;
	binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings    = &binding;
	vkCreateDescriptorSetLayout(dev->vkdev, &layoutInfo, dev->vkalloc, &sampler->descLayout);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = dev->vkdescriptorpool[3];
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts        = &sampler->descLayout;
	vkAllocateDescriptorSets(dev->vkdev, &allocInfo, &sampler->descSet);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView   = NULL;
	imageInfo.sampler     = sampler->sampler;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet          = sampler->descSet;
	write.dstBinding      = 0;
	write.dstArrayElement = 0;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
	write.descriptorCount = 1;
	write.pImageInfo      = &imageInfo;
	vkUpdateDescriptorSets(dev->vkdev, 1, &write, 0, nullptr);

	return sampler;
}

void R_DestroySampler(RSampler* sampler) {
	vkFreeDescriptorSets(sampler->dev->vkdev, sampler->dev->vkdescriptorpool[3], 1, &sampler->descSet);
	vkDestroyDescriptorSetLayout(sampler->dev->vkdev, sampler->descLayout, sampler->dev->vkalloc);
	vkDestroySampler(sampler->dev->vkdev, sampler->sampler, sampler->dev->vkalloc);
}