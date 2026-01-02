#include <rshared.h>
#include "rendertypesvk.h"
#include "imgui_impl_vulkan.h"

void R_CmdBeginRenderpass(RCommandBuffer* cmdbuff, RRenderpassBeginInfo* info) {
	VkRenderPassBeginInfo renderPassInfo = {};
	VkClearValue clearColor[8] = {};

	VkViewport viewport = {};
	VkRect2D scissor = {};

	info->renderpass->depthEnabled;
	Uint32 rt_count = 1;

	if (info) {
		rt_count = info->renderpass->rt_count;
		for (Uint32 i = 0; i < rt_count; i++) {
			clearColor[i].color.float32[0] = info->clearinfo->color->r;
			clearColor[i].color.float32[1] = info->clearinfo->color->g;
			clearColor[i].color.float32[2] = info->clearinfo->color->b;
			clearColor[i].color.float32[3] = info->clearinfo->color->a;
		}
		if (info->renderpass->depthEnabled) {
			clearColor[rt_count].depthStencil.depth   = info->clearinfo->depth;
			clearColor[rt_count].depthStencil.stencil = info->clearinfo->stencil;
			rt_count++;
		}

		viewport.width  = info->framebuffer->width;
		viewport.height = info->framebuffer->height;// -info->framebuffer->height;
		viewport.x = 0;
		viewport.y = 0;// info->framebuffer->height;

		scissor.extent.width  = info->framebuffer->width;
		scissor.extent.height = info->framebuffer->height;

		renderPassInfo.renderPass      = info->renderpass->renderpass;
		renderPassInfo.framebuffer     = info->framebuffer->framebuffer;
		renderPassInfo.clearValueCount = rt_count;
		renderPassInfo.pClearValues    = clearColor;
	} else {

		viewport.width  = cmdbuff->dev->vkextent.width;
		viewport.height = cmdbuff->dev->vkextent.height;
		viewport.x = 0;
		viewport.y = 0;

		scissor.extent.width  = viewport.width;
		scissor.extent.height = viewport.height;

		renderPassInfo.renderPass      = cmdbuff->dev->imguirenderpass;
		renderPassInfo.framebuffer     = cmdbuff->dev->vkframebuffers[cmdbuff->dev->vkcurrentimage];
		renderPassInfo.clearValueCount = 0;
		renderPassInfo.pClearValues    = NULL;
	}

	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = cmdbuff->dev->vkextent;

	vkCmdBeginRenderPass(cmdbuff->cmdbuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	vkCmdSetViewport(cmdbuff->cmdbuffer, 0, 1, &viewport);

	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(cmdbuff->cmdbuffer, 0, 1, &scissor);
}

void R_CmdEndRenderpass(RCommandBuffer* cmdbuff) {
	vkCmdEndRenderPass(cmdbuff->cmdbuffer);
}

void R_CmdBindPipeline(RCommandBuffer* cmdbuff, RPipeline* pl) {
	cmdbuff->pipeline = pl;
	vkCmdBindPipeline(cmdbuff->cmdbuffer, pl->type, pl->pipeline);
}

void R_CmdBindVertexBuffer(RCommandBuffer* cmdbuff, RBuffer* vb, Uint32 slot, Uint32 stride) {
	const VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmdbuff->cmdbuffer, slot, 1, &vb->buffer, &offset);
}

void R_CmdBindIndexBuffer(RCommandBuffer* cmdbuff, RBuffer* ib, IndexType isize) {
	vkCmdBindIndexBuffer(cmdbuff->cmdbuffer, ib->buffer, 0, GetVkIndexType(isize));
}

void R_CmdBindDescriptorSets(RCommandBuffer* cmdbuff, RBindDescriptorSetsInfo* info) {
	for (Uint32 i = 0; i < info->count; i++) {
		vkCmdBindDescriptorSets(
			cmdbuff->cmdbuffer, cmdbuff->pipeline->type, cmdbuff->pipeline->layout,
			info->startslot + i, 1, &info->sets[i]->set,
			0, NULL);
	}
}

void R_CmdBindSampler(RCommandBuffer* cmdbuff, RSampler* sampler, Uint32 slot, Uint32 stage) {
	vkCmdBindDescriptorSets(cmdbuff->cmdbuffer, cmdbuff->pipeline->type, cmdbuff->pipeline->layout, slot, 1, &sampler->descSet, 0, NULL);
}

void R_CmdDrawIndexed(RCommandBuffer* cmdbuff, Uint32 idxcount, Uint32 idxstart) {
	vkCmdDrawIndexed(cmdbuff->cmdbuffer, idxcount, 1, idxstart, 0, 0);
	cmdbuff->dev->draw_calls++;
}

void R_CmdPushConstants(RCommandBuffer* cmdbuff, void* buffer, Uint32 size, Uint32 stage) {
	Uint32 offset = 0;
	if (stage == RG_SHADER_TYPE_PIXEL) {
		offset = 128;
	}
	vkCmdPushConstants(cmdbuff->cmdbuffer, cmdbuff->pipeline->layout, GetShaderStage(stage), offset, size, buffer);
}

void R_CmdImGuiRenderDrawData(RCommandBuffer* cmdbuff, void* drawData) {
	ImGui_ImplVulkan_RenderDrawData((ImDrawData*)drawData, cmdbuff->cmdbuffer, NULL);
}

void R_CmdDispatch(RCommandBuffer* cmdbuff, Uint32 gc_x, Uint32 gc_y, Uint32 gc_z) {
	vkCmdDispatch(cmdbuff->cmdbuffer, gc_x, gc_y, gc_z);
	cmdbuff->dev->dispatch_calls++;
}

void R_CmdUseImage(RCommandBuffer* cmdbuff, RImage* image, Uint32 usage) {
	VkImageLayout newLayout = GetImageLayout(usage);

	// No need to change layout
	if (image->layout == newLayout) { return; }

	VkPipelineStageFlagBits srcStage = GetImagePipelineStage(image->layout);
	VkPipelineStageFlagBits dstStage = GetImagePipelineStage(newLayout);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = GetImageAccess(image->layout);
	barrier.dstAccessMask = GetImageAccess(newLayout);
	barrier.oldLayout = image->layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	if (image->format == RG_FORMAT_D32) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	if (image->format == RG_FORMAT_D24S8) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;

	vkCmdPipelineBarrier(cmdbuff->cmdbuffer, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &barrier);

	image->layout = newLayout;
}