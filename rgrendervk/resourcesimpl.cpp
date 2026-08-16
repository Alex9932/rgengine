#include <rshared.h>
#include <vma/vk_mem_alloc.h>
#include "rendertypesvk.h"

#include <mutex>

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

static Uint32 GetMipmapLevels(Uint32 w, Uint32 h) {
	Float32 x = SDL_max(w, h);
	// log2(x) = ln(x) / ln(2)
	return (Uint32)SDL_floorf(SDL_logf(x) / SDL_logf(2));
}

static std::mutex t_lock;
static void CopyToImage(RBuffer* src, RImage* dst, RImageCreateInfo* info) {

	RRenderDevice* dev = src->dev;
	VkCommandBuffer cmdbuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = dev->vktransfercommandpool;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	Uint32 mipLevels = GetMipmapLevels(info->width, info->height);

	t_lock.lock();

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
	barrier.srcQueueFamilyIndex = dev->vktransferqueuefamily;
	barrier.dstQueueFamilyIndex = dev->vktransferqueuefamily;
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
#if 0
	// Transition image layout to SHADER_READ_ONLY_OPTIMAL
	barrier = {};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = dev->vktransferqueuefamily;
	barrier.dstQueueFamilyIndex = dev->vkqueuefamily;
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
#endif

	if (RG_CHECK_FLAG(info->flags, RG_IMAGE_FLAG_GENERATE_MIPMAPS)) {
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = dst->image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;
		int32_t mipWidth = info->width;
		int32_t mipHeight = info->height;

		for (Uint32 i = 1; i < mipLevels; i++) {

			// Layout for SOURCE mip level
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier(cmdbuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr, 0, nullptr,
				1, &barrier);

			// Layout for DST mip level
			barrier.subresourceRange.baseMipLevel = i;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(cmdbuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr, 0, nullptr,
				1, &barrier);

			// Downscale image
			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;
			vkCmdBlitImage(cmdbuffer,
				dst->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit, VK_FILTER_LINEAR);

			// Restore SOURCE mip level layout for shader read
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.srcQueueFamilyIndex = dev->vktransferqueuefamily;
			barrier.dstQueueFamilyIndex = dev->vkqueuefamily;

			vkCmdPipelineBarrier(cmdbuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr, 0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

	} else {
		mipLevels = 1;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.srcQueueFamilyIndex = dev->vktransferqueuefamily;
	barrier.dstQueueFamilyIndex = dev->vkqueuefamily;
	vkCmdPipelineBarrier(cmdbuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr, 0, nullptr,
		1, &barrier);

	vkEndCommandBuffer(cmdbuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdbuffer;
	vkQueueSubmit(dev->vktransferqueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(dev->vktransferqueue);



	vkFreeCommandBuffers(dev->vkdev, dev->vktransfercommandpool, 1, &cmdbuffer);
	dst->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	dst->usage = RG_IMAGE_USAGE_SHADER_READ_ONLY;

	t_lock.unlock();
}

static void CopyToBuffer(RBuffer* src, RBuffer* dst, size_t len) {
	RRenderDevice* dev = src->dev;
	VkCommandBuffer cmdbuffer;

	t_lock.lock();

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = dev->vktransfercommandpool;
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
	copyRegion.size      = len;
	vkCmdCopyBuffer(cmdbuffer, src->buffer, dst->buffer, 1, &copyRegion);

	vkEndCommandBuffer(cmdbuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdbuffer;
	vkQueueSubmit(dev->vktransferqueue, 1, &submitInfo, NULL);

	vkQueueWaitIdle(dev->vktransferqueue);
	vkFreeCommandBuffers(dev->vkdev, dev->vktransfercommandpool, 1, &cmdbuffer);

	t_lock.unlock();

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
		//if (info->access != RG_BUFFER_ACCESS_GPU_ONLY) {
			// For CPU_WRITE buffers
			RUpdateBufferInfo updateInfo = {};
			updateInfo.handle = buffer;
			updateInfo.offset = 0;
			updateInfo.length = info->length;
			updateInfo.data   = info->initialData;
			R_UpdateBuffer(&updateInfo);
		//} else {
		//	// For GPU_ONLY buffers
		//	RBufferCreateInfo stagingInfo = {};
		//	stagingInfo.length = info->length;
		//	stagingInfo.type   = RG_BUFFER_TYPE_VK_TSRC;
		//	stagingInfo.usage  = RG_BUFFER_USAGE_DYNAMIC;
		//	stagingInfo.access = RG_BUFFER_ACCESS_CPU_WRITE;
		//	stagingInfo.initialData = info->initialData;
		//	RBuffer* stagingBuffer = R_CreateBuffer(dev, &stagingInfo);
		//	CopyToBuffer(stagingBuffer, buffer, info);
		//	R_DestroyBuffer(stagingBuffer);
		//}
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

			// For GPU_ONLY buffers
		RBufferCreateInfo stagingInfo = {};
		stagingInfo.length = info->length;
		stagingInfo.type = RG_BUFFER_TYPE_VK_TSRC;
		stagingInfo.usage = RG_BUFFER_USAGE_DYNAMIC;
		stagingInfo.access = RG_BUFFER_ACCESS_CPU_WRITE;
		stagingInfo.initialData = info->data;
		RBuffer* stagingBuffer = R_CreateBuffer(dev, &stagingInfo);
		CopyToBuffer(stagingBuffer, info->handle, info->length);
		R_DestroyBuffer(stagingBuffer);

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
	image->layout = VK_IMAGE_LAYOUT_UNDEFINED;
	image->usage  = RG_IMAGE_USAGE_UNDEFINED;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	imageInfo.format        = GetImageFormat(info->format);
	imageInfo.extent.width  = info->width;
	imageInfo.extent.height = info->height;
	imageInfo.extent.depth  = 1;
	imageInfo.arrayLayers   = 1;
	imageInfo.mipLevels     = 1;
	imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (info->format == RG_FORMAT_D24S8 || info->format == RG_FORMAT_D32) {
		imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else {
		imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (RG_CHECK_FLAG(info->flags, RG_IMAGE_FLAG_GENERATE_MIPMAPS)) {
			imageInfo.mipLevels = GetMipmapLevels(info->width, info->height);
			//rgLogInfo(RG_LOG_RENDER, "[VK] Generate %d mipmaps for image %ld", imageInfo.mipLevels, image);
		}
	}
	imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	vmaCreateImage(dev->vmaallocator, &imageInfo, &allocCreateInfo, &image->image, &image->allocation, NULL);

	VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;

#if 0
	if (info->format == RG_FORMAT_D24S8) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (info->format == RG_FORMAT_D32) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
#else
	if (info->format == RG_FORMAT_D24S8 || info->format == RG_FORMAT_D32) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
#endif

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                           = image->image;
	viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format                          = GetImageFormat(info->format);
	viewInfo.subresourceRange.aspectMask     = aspect;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = imageInfo.mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = 1;
	vkCreateImageView(dev->vkdev, &viewInfo, dev->vkalloc, &image->view);

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

	dev->imageMemLen += image->length;

	return image;
}

void R_DestroyImage(RImage* image) {
	RRenderDevice* dev = image->dev;
	vkDestroyImageView(dev->vkdev, image->view, dev->vkalloc);
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
		attachments[i] = info->rts[i]->view;
	}

	if (info->dsv) {
		attachments[info->rt_count] = info->dsv->view;
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
	for (Uint32 i = 0; i < R_VK_FRAMES_IN_FLIGHT; i++) {
		vkCreateFramebuffer(dev->vkdev, &fbInfo, dev->vkalloc, &fb->framebuffer[i]);
	}

	return fb;
}

void R_DestroyFramebuffer(RFramebuffer* fb) {
	RRenderDevice* dev = fb->dev;
	for (Uint32 i = 0; i < R_VK_FRAMES_IN_FLIGHT; i++) {
		vkDestroyFramebuffer(dev->vkdev, fb->framebuffer[i], dev->vkalloc);
	}
	dev->allocator->Deallocate(fb);
}

RCommandBuffer* R_CreateCommandBuffer(RRenderDevice* dev, RCommandBufferCreateInfo* info) {
	RCommandBuffer* buffer = (RCommandBuffer*)dev->allocator->Allocate(sizeof(RCommandBuffer));
	buffer->dev = dev;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool        = dev->vkcommandpool;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = R_VK_FRAMES_IN_FLIGHT;
	vkAllocateCommandBuffers(dev->vkdev, &allocInfo, buffer->cmdbuffer);

	return buffer;
}

void R_DestroyCommandBuffer(RCommandBuffer* buffer) {
	RRenderDevice* dev = buffer->dev;
	vkFreeCommandBuffers(dev->vkdev, dev->vkcommandpool, R_VK_FRAMES_IN_FLIGHT, buffer->cmdbuffer);
	dev->allocator->Deallocate(buffer);
}

void R_ResetCommandBuffer(RCommandBuffer* buffer) {
	vkResetCommandBuffer(buffer->cmdbuffer[buffer->dev->vkcurrentimage], 0);
}

void R_BeginCommandBuffer(RCommandBuffer* buffer) {
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(buffer->cmdbuffer[buffer->dev->vkcurrentimage], &info);
}

void R_EndCommandBuffer(RCommandBuffer* buffer) {
	vkEndCommandBuffer(buffer->cmdbuffer[buffer->dev->vkcurrentimage]);
}

static void SubmitCommandBuffer(RRenderDevice* dev, VkCommandBuffer cmdbuffer) {
	static std::mutex func_lock;

	func_lock.lock();
	VkSubmitInfo submitInfo = {};

	VkPipelineStageFlags f[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdbuffer;

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &dev->cmdbuffsemaphores[dev->vkcurrentimage][dev->cmdsemaphore];
	submitInfo.pWaitDstStageMask = f;

	dev->cmdsemaphore++;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &dev->cmdbuffsemaphores[dev->vkcurrentimage][dev->cmdsemaphore];
	vkQueueSubmit(dev->vkqueue, 1, &submitInfo, NULL);
	//vkQueueWaitIdle(dev->vkqueue);
	func_lock.unlock();

}

void R_SubmitCommandBuffer(RCommandBufferSubmitInfo* info) {
	// TODO: Support semaphores and fences
	RRenderDevice* dev = info->buffer->dev;
	SubmitCommandBuffer(dev, info->buffer->cmdbuffer[dev->vkcurrentimage]);
}

#if 0
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
#endif

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
	sampInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampInfo.addressModeU     = GetSamplerAddressMode(info->addressModeU);
	sampInfo.addressModeV     = GetSamplerAddressMode(info->addressModeV);
	sampInfo.addressModeW     = GetSamplerAddressMode(info->addressModeW);
	sampInfo.anisotropyEnable = (info->filterMode == RG_SAMPLER_FILTER_ANISOTROPIC && dev->isAnisotropicEnabled) ? VK_TRUE : VK_FALSE;
	sampInfo.maxAnisotropy    = info->maxAnisotropy;
	sampInfo.borderColor      = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	//sampInfo.unnormalizedCoordinates = VK_FALSE;
	sampInfo.compareEnable    = VK_FALSE;
	sampInfo.compareOp        = VK_COMPARE_OP_ALWAYS;
	sampInfo.mipLodBias       = -0.75f;
	sampInfo.minLod           = 0.0f;
	sampInfo.maxLod           = 8.0f;
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
	allocInfo.descriptorPool     = dev->vkdescriptorpool;
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
	RRenderDevice* dev = sampler->dev;
	vkFreeDescriptorSets(dev->vkdev, dev->vkdescriptorpool, 1, &sampler->descSet);
	vkDestroyDescriptorSetLayout(dev->vkdev, sampler->descLayout, dev->vkalloc);
	vkDestroySampler(dev->vkdev, sampler->sampler, sampler->dev->vkalloc);

	dev->allocator->Deallocate(sampler);
}

RDescriptorSet* R_CreateDescriptorSet(RRenderDevice* dev, RDescriptorSetCreateInfo* info) {
	RDescriptorSet* ds = (RDescriptorSet*)dev->allocator->Allocate(sizeof(RDescriptorSet));

	ds->dev = dev;

	VkDescriptorSetLayoutBinding bindings[16] = {};
	for (size_t i = 0; i < info->binding_count; i++) {
		bindings[i].binding = info->bindings[i].binding;
		bindings[i].descriptorCount = 1;
		bindings[i].descriptorType  = GetDescriptorType(info->bindings[i].type);
		bindings[i].stageFlags = GetShaderStage(info->bindings[i].stage);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = info->binding_count;
	layoutInfo.pBindings = bindings;
	vkCreateDescriptorSetLayout(dev->vkdev, &layoutInfo, dev->vkalloc, &ds->layout);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = dev->vkdescriptorpool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &ds->layout;
	for (size_t i = 0; i < R_VK_FRAMES_IN_FLIGHT; i++) {
		vkAllocateDescriptorSets(dev->vkdev, &allocInfo, &ds->set[i]);
	}

	VkWriteDescriptorSet   writes[16]  = {};
	VkDescriptorImageInfo  images[16]  = {};
	VkDescriptorBufferInfo buffers[16] = {};

	for (size_t j = 0; j < R_VK_FRAMES_IN_FLIGHT; j++) {
		for (size_t i = 0; i < info->binding_count; i++) {

			writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[i].dstSet = ds->set[j];
			writes[i].dstBinding = info->bindings[i].binding;
			writes[i].dstArrayElement = 0;
			writes[i].descriptorCount = 1;
			writes[i].descriptorType = GetDescriptorType(info->bindings[i].type);

			if (info->bindings[i].type == RG_DESCRIPTOR_TYPE_IMAGE) {
				images[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				images[i].imageView = info->bindings[i].image->view;
				writes[i].pImageInfo = &images[i];
			}
			else {
				buffers[i].buffer = info->bindings[i].buffer->buffer;
				buffers[i].offset = 0;
				buffers[i].range = VK_WHOLE_SIZE;
				writes[i].pBufferInfo = &buffers[i];
			}

		}
		vkUpdateDescriptorSets(dev->vkdev, info->binding_count, writes, 0, nullptr);
	}
	return ds;
}

void R_DestroyDescriptorSet(RDescriptorSet* ds) {
	RRenderDevice* dev = ds->dev;

	vkQueueWaitIdle(dev->vkqueue);

	vkFreeDescriptorSets(dev->vkdev, dev->vkdescriptorpool, R_VK_FRAMES_IN_FLIGHT, ds->set);
	vkDestroyDescriptorSetLayout(dev->vkdev, ds->layout, dev->vkalloc);

	dev->allocator->Deallocate(ds);
}