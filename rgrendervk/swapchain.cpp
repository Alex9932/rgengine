#include "swapchain.h"

#include <allocator.h>
#include <rgvector.h>
#include <engine.h>

#include <SDL3/SDL_vulkan.h>

static VkSurfaceFormatKHR ChooseFormat(VkSurfaceFormatKHR* formats, Uint32 count) {
	VkSurfaceFormatKHR current;
	for (Uint32 i = 0; i < count; i++) {
		current = formats[i];
		if (current.format == VK_FORMAT_R8G8B8A8_UNORM && current.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return current;
		}
	}
	return formats[0];
}

static VkPresentModeKHR ChoosePresentMode(VkPresentModeKHR* modes, Uint32 count) {
	// Find MAILBOX for high performance
	for (Uint32 i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) { return VK_PRESENT_MODE_MAILBOX_KHR; }
	}

	// OR IMMEDIATE
	for (Uint32 i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) { return VK_PRESENT_MODE_IMMEDIATE_KHR; }
	}

	// Kurwa! Nu vse pizdec nahuy...
	// (╯°□°）╯︵ ┻━┻
	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseExtent(VkSurfaceCapabilitiesKHR capabilities, SDL_Window* hwnd) {

	ivec2 size = {};
	SDL_GetWindowSize(hwnd, &size.x, &size.y);

	VkExtent2D extent = {};
	extent.width  = SDL_clamp(size.x, capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
	extent.height = SDL_clamp(size.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return extent;
}

static void MakeSwapchain(RRenderDevice* device) {

	VkPhysicalDevice pdev = device->vkpdev;
	VkDevice         dev  = device->vkdev;

	// Create surface
	SDL_Vulkan_CreateSurface(device->hwnd, device->vkctx, device->vkalloc, &device->vksurface);

	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, device->vksurface, &capabilities);

	// Get Formats & modes
	Uint32 fcount = 0;
	Uint32 mcount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, device->vksurface, &fcount, NULL);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, device->vksurface, &mcount, NULL);

	// TODO: Use renderer's allocator
	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)rg_malloc(sizeof(VkSurfaceFormatKHR) * fcount);
	VkPresentModeKHR* modes = (VkPresentModeKHR*)rg_malloc(sizeof(VkPresentModeKHR) * mcount);

	vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, device->vksurface, &fcount, formats);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, device->vksurface, &mcount, modes);

	device->vkswapchainformat = ChooseFormat(formats, fcount);
	device->vkpresentmode = ChoosePresentMode(modes, mcount);

	rg_free(formats);
	rg_free(modes);

	device->vkextent = ChooseExtent(capabilities, device->hwnd);

	rgLogInfo(RG_LOG_RENDER, "VK: Swapchain format: f=%d s=%d", device->vkswapchainformat.format, device->vkswapchainformat.colorSpace);
	rgLogInfo(RG_LOG_RENDER, "VK: Swapchain present: %d", device->vkpresentmode);
	rgLogInfo(RG_LOG_RENDER, "VK: Swapchain size: %dx%d", device->vkextent.width, device->vkextent.height);

	// Image count

	device->vkimagescount = capabilities.minImageCount + 1;
	device->vkimagescount = SDL_min(device->vkimagescount, R_VK_FRAMES_IN_FLIGHT);
	//Uint32 maximages = SDL_min(capabilities.maxImageCount, R_VK_FRAMES_IN_FLIGHT);
	//if (capabilities.maxImageCount > 0 && device->vkimagescount > maximages) {
	//	device->vkimagescount = maximages;
	//}

	// Present queue

	Uint32 qcount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pdev, &qcount, nullptr);
	for (Uint32 i = 0; i < qcount; i++) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(pdev, i, device->vksurface, &presentSupport);
		if (presentSupport) { device->vkpresentqueue = i; break; }
	}

	Uint32 queueFamilyIndices[] = { device->vkqueuefamily, device->vkpresentqueue };

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface          = device->vksurface;
	createInfo.minImageCount    = device->vkimagescount;
	createInfo.imageFormat      = device->vkswapchainformat.format;
	createInfo.imageColorSpace  = device->vkswapchainformat.colorSpace;
	createInfo.imageExtent      = device->vkextent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	if (device->vkqueuefamily != device->vkpresentqueue) {
		// Different queues
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else {
		// Same queue
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = device->vkpresentmode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(dev, &createInfo, device->vkalloc, &device->vkswapchain) != VK_SUCCESS) {
		RG_ERROR_MSG("Vulkan swapchain error!");
	}

	Uint32 icount = 0;
	vkGetSwapchainImagesKHR(dev, device->vkswapchain, &icount, NULL);
	vkGetSwapchainImagesKHR(dev, device->vkswapchain, &icount, device->vkswapimages);
	if (icount != device->vkimagescount) {
		rgLogError(RG_LOG_RENDER, "Image count: created=%d needed=%d", icount, device->vkimagescount);
		RG_ERROR_MSG("Vulkan swapchain images error!");
	}

	// Make VkImageView
	for (size_t i = 0; i < device->vkimagescount; i++) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image        = device->vkswapimages[i];
		viewInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format       = device->vkswapchainformat.format;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel   = 0;
		viewInfo.subresourceRange.levelCount     = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount     = 1;

		if (vkCreateImageView(dev, &viewInfo, device->vkalloc, &device->vkimageviews[i]) != VK_SUCCESS) {
			RG_ERROR_MSG("Vulkan swapchain imageview error!");
		}
	}

	rgLogInfo(RG_LOG_RENDER, "Created %d swapchain images", device->vkimagescount);

}

void MakeSwapchainFramebuffer(RRenderDevice* device) {
	VkDevice dev = device->vkdev;

	// Make VkFramebuffer
	for (size_t i = 0; i < device->vkimagescount; i++) {
		VkImageView attachments[] = { device->vkimageviews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass      = device->imguirenderpass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments    = attachments;
		framebufferInfo.width           = device->vkextent.width;
		framebufferInfo.height          = device->vkextent.height;
		framebufferInfo.layers          = 1;
		if (vkCreateFramebuffer(dev, &framebufferInfo, device->vkalloc, &device->vkframebuffers[i]) != VK_SUCCESS) {
			RG_ERROR_MSG("Vulkan swapchain framebuffer error!");
		}
	}
}

void CreateSwapchain(RRenderDevice* dev) {
	MakeSwapchain(dev);
}

void DestroySwapchain(RRenderDevice* device) {
	VkDevice dev = device->vkdev;
	for (size_t i = 0; i < device->vkimagescount; i++) {
		vkDestroyFramebuffer(dev, device->vkframebuffers[i], device->vkalloc);
		vkDestroyImageView(dev, device->vkimageviews[i], device->vkalloc);
	}
	vkDestroySwapchainKHR(dev, device->vkswapchain, device->vkalloc);
	SDL_Vulkan_DestroySurface(device->vkctx, device->vksurface, device->vkalloc);
}

void ResizeSwapchain(RRenderDevice* dev) {
	vkDeviceWaitIdle(dev->vkdev);
	DestroySwapchain(dev);
	MakeSwapchain(dev);
	MakeSwapchainFramebuffer(dev);
}