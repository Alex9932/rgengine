#include "swapchain.h"

#include <allocator.h>
#include <rgvector.h>
#include <engine.h>

#include <SDL3/SDL_vulkan.h>

#include "vk.h"

static VkSurfaceKHR       vk_surface          = NULL;
static VkSurfaceFormatKHR vk_swapchainformat  = {};
static VkPresentModeKHR   vk_presentmode      = VK_PRESENT_MODE_IMMEDIATE_KHR;
static VkExtent2D         vk_extent           = {};
static VkSwapchainKHR     vk_swapchain        = NULL;
static Uint32             vk_imagescount      = 0;
static VkImage            vk_swapimages[16]   = {};
static VkImageView        vk_imageviews[16]   = {};
static VkFramebuffer      vk_framebuffers[16] = {};
static Uint32             vk_presentqueue     = 0;

static SDL_Window*        vk_hwnd             = NULL;

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
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) { return modes[i]; }
	}

	// OR IMMEDIATE
	for (Uint32 i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) { return modes[i]; }
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

static void MakeSwapchain() {

	VkPhysicalDevice pdev = VK_GetPhysicalDevice();
	VkDevice         dev = VK_GetDevice();

	// Create surface
	SDL_Vulkan_CreateSurface(vk_hwnd, VK_GetInstance(), NULL, &vk_surface);

	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, vk_surface, &capabilities);

	// Get Formats & modes
	Uint32 fcount = 0;
	Uint32 mcount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, vk_surface, &fcount, NULL);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, vk_surface, &mcount, NULL);

	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)rg_malloc(sizeof(VkSurfaceFormatKHR) * fcount);
	VkPresentModeKHR* modes = (VkPresentModeKHR*)rg_malloc(sizeof(VkPresentModeKHR) * mcount);

	vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, vk_surface, &fcount, formats);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, vk_surface, &mcount, modes);

	vk_swapchainformat = ChooseFormat(formats, fcount);
	vk_presentmode = ChoosePresentMode(modes, mcount);

	rg_free(formats);
	rg_free(modes);

	vk_extent = ChooseExtent(capabilities, vk_hwnd);

	rgLogInfo(RG_LOG_RENDER, "VK: Swapchain format: f=%d s=%d", vk_swapchainformat.format, vk_swapchainformat.colorSpace);
	rgLogInfo(RG_LOG_RENDER, "VK: Swapchain present: %d", vk_presentmode);
	rgLogInfo(RG_LOG_RENDER, "VK: Swapchain size: %dx%d", vk_extent.width, vk_extent.height);

	// Image count

	vk_imagescount = capabilities.minImageCount + 1;

	if (capabilities.maxImageCount > 0 && vk_imagescount > capabilities.maxImageCount) {
		vk_imagescount = capabilities.maxImageCount;
	}

	// Present queue

	Uint32 qcount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pdev, &qcount, nullptr);
	for (Uint32 i = 0; i < qcount; i++) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(pdev, i, vk_surface, &presentSupport);
		if (presentSupport) { vk_presentqueue = i; break; }
	}

	Uint32 queueFamilyIndices[] = { VK_GetQueueFamily(), vk_presentqueue };

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = vk_surface;
	createInfo.minImageCount = vk_imagescount;
	createInfo.imageFormat = vk_swapchainformat.format;
	createInfo.imageColorSpace = vk_swapchainformat.colorSpace;
	createInfo.imageExtent = vk_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	if (VK_GetQueueFamily() != vk_presentqueue) {
		// Different queues
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		// Same queue
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = vk_presentmode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(dev, &createInfo, nullptr, &vk_swapchain) != VK_SUCCESS) {
		RG_ERROR_MSG("Vulkan swapchain error!");
	}

	Uint32 icount = 0;
	vkGetSwapchainImagesKHR(dev, vk_swapchain, &icount, NULL);
	vkGetSwapchainImagesKHR(dev, vk_swapchain, &icount, vk_swapimages);
	if (icount != vk_imagescount) {
		rgLogError(RG_LOG_RENDER, "Image count: created=%d needed=%d", icount, vk_imagescount);
		RG_ERROR_MSG("Vulkan swapchain images error!");
	}

	// Make VkImageView
	for (size_t i = 0; i < vk_imagescount; i++) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = vk_swapimages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = vk_swapchainformat.format;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(dev, &viewInfo, nullptr, &vk_imageviews[i]) != VK_SUCCESS) {
			RG_ERROR_MSG("Vulkan swapchain imageview error!");
		}
	}

	rgLogInfo(RG_LOG_RENDER, "Created %d swapchain images", vk_imagescount);

}

void MakeSwapchainFramebuffer() {
	VkDevice dev = VK_GetDevice();

	// Make VkFramebuffer
	for (size_t i = 0; i < vk_imagescount; i++) {
		VkImageView attachments[] = { vk_imageviews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass      = VK_GetRenderpass();
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments    = attachments;
		framebufferInfo.width           = vk_extent.width;
		framebufferInfo.height          = vk_extent.height;
		framebufferInfo.layers          = 1;
		if (vkCreateFramebuffer(dev, &framebufferInfo, nullptr, &vk_framebuffers[i]) != VK_SUCCESS) {
			RG_ERROR_MSG("Vulkan swapchain framebuffer error!");
		}
	}
}

void CreateSwapchain(SDL_Window* hwnd) {
	vk_hwnd = hwnd;
	MakeSwapchain();

}

void DestroySwapchain() {
	VkDevice dev = VK_GetDevice();
	for (size_t i = 0; i < vk_imagescount; i++) {
		vkDestroyImageView(dev, vk_imageviews[i], nullptr);
	}
	vkDestroySwapchainKHR(dev, vk_swapchain, nullptr);
}

void ResizeSwapchain() {
	vkDeviceWaitIdle(VK_GetDevice());
	DestroySwapchain();
	MakeSwapchain();
	MakeSwapchainFramebuffer();
}

VkFramebuffer GetSwapchainFramebuffer(Uint32 idx) { return vk_framebuffers[idx]; }
VkExtent2D GetSwapchainExtent() { return vk_extent; }
Uint32 GetSwapchainImageCount() { return vk_imagescount; }
VkSwapchainKHR GetSwapchain() { return vk_swapchain; }
VkSurfaceFormatKHR GetSwapchainFormat() { return vk_swapchainformat; }