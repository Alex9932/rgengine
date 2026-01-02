#include <rshared.h>
#include "rendertypesvk.h"
#include "swapchain.h"

#include <allocator.h>
#include <engine.h>
#include <event.h>

#include <SDL3/SDL_vulkan.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_impl_vulkan.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

using namespace Engine;

static Uint32 vk_version = VK_API_VERSION_1_3;

static Uint32 flags = 0;

SDL_Window* R_ShowWindow(Uint32 w, Uint32 h) {
	SDL_Window* sdl_hwnd = SDL_CreateWindow("rgEngine", w, h, SDL_WINDOW_VULKAN);
	SDL_SetWindowPosition(sdl_hwnd, 5, 5);
	return sdl_hwnd;
}

void R_Setup() {

}

static Bool _EventHandler(SDL_Event* event, void* data) {
	return true;
}

#if R_VKRENDER_DEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL R_DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	RRenderDevice* dev = (RRenderDevice*)pUserData;

	rgLogWarn(RG_LOG_RENDER, "Vulkan DBG: ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~");
	rgLogWarn(RG_LOG_RENDER, "Vulkan DBG: Swap image: %d", dev->vkcurrentimage);
	rgLogWarn(RG_LOG_RENDER, "Vulkan DBG: %s", pCallbackData->pMessage);
	rgLogWarn(RG_LOG_RENDER, "Vulkan DBG: VK objects (%d)", pCallbackData->objectCount);
	for (Uint32 i = 0; i < pCallbackData->objectCount; i++) {
		const VkDebugUtilsObjectNameInfoEXT* obj = &pCallbackData->pObjects[i];
		rgLogWarn(RG_LOG_RENDER, "Vulkan DBG: + %s (%x)", obj->pObjectName, obj->objectHandle);
	}

	rgLogWarn(RG_LOG_RENDER, "Vulkan DBG: Command buffers (%d)", pCallbackData->cmdBufLabelCount);
	for (Uint32 i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
		const VkDebugUtilsLabelEXT* cb = &pCallbackData->pCmdBufLabels[i];
		rgLogWarn(RG_LOG_RENDER, "Vulkan DBG: + %s", cb->pLabelName);
	}

	return VK_FALSE;
}

#endif

RRenderDevice* R_CreateDevice(RRenderSetupInfo* info) {
	flags = info->flags;

	// Make new allocator for rendering device
	STDAllocator* alloc = RG_NEW(STDAllocator)("VK allocator");

	// Create device
	RRenderDevice* device = RG_NEW_CLASS(alloc, RRenderDevice);
	SDL_memset(device, 0, sizeof(RRenderDevice)); // Reset device struct

	device->flags     = info->flags;
	device->allocator = alloc;
	device->hwnd      = info->hwnd;

	device->vkalloc   = NULL;

	SDL_SetWindowTitle(device->hwnd, "rgEngine - Vulkan");
	Engine::RegisterEventHandler(_EventHandler, device);

	// Setup vulkan

#if R_VKRENDER_DEBUG
	Uint32 instanceLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
	VkLayerProperties* layers = (VkLayerProperties*)rg_malloc(sizeof(VkLayerProperties) * instanceLayerCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, layers);
	rgLogInfo(RG_LOG_RENDER, "Vulkan layers: %d", instanceLayerCount);
	for (Uint32 i = 0; i < instanceLayerCount; i++) {
		rgLogInfo(RG_LOG_RENDER, "-> %s", layers[i].layerName);
	}
	rg_free(layers);
#endif

	Uint32 sdlExtensionCount = 0;
	String* sdlExtensions = (String*)SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);


	VkApplicationInfo appInfo = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "test renderer"; // TODO: replace this
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "rgEngine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = vk_version;

	VkInstanceCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
#if R_VKRENDER_DEBUG
	std::vector<String> exts;
	for (Uint32 i = 0; i < sdlExtensionCount; i++) {
		exts.push_back(sdlExtensions[i]);
	}
	exts.push_back("VK_EXT_debug_utils");
	
    createInfo.enabledExtensionCount   = exts.size();
    createInfo.ppEnabledExtensionNames = exts.data();
#else
    createInfo.enabledExtensionCount   = sdlExtensionCount;
    createInfo.ppEnabledExtensionNames = sdlExtensions;
#endif

#if R_VKRENDER_DEBUG
	String validationlayer = "VK_LAYER_KHRONOS_validation";
	createInfo.enabledLayerCount       = 1;
	createInfo.ppEnabledLayerNames     = &validationlayer;
#else
	createInfo.enabledLayerCount       = 0;
	createInfo.ppEnabledLayerNames     = NULL;
#endif

	if (vkCreateInstance(&createInfo, device->vkalloc, &device->vkctx) != VK_SUCCESS) {
		RG_ERROR_MSG("Vulkan instance error!");
	}

#if R_VKRENDER_DEBUG

	VkDebugUtilsMessengerCreateInfoEXT msgCreateInfo = {};
	msgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	msgCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	msgCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	msgCreateInfo.pfnUserCallback = R_DebugCallback;
	msgCreateInfo.pUserData = device;
	device->debugMessenger;

	auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(device->vkctx, "vkCreateDebugUtilsMessengerEXT");

	if (vkCreateDebugUtilsMessengerEXT != NULL) {
		if (vkCreateDebugUtilsMessengerEXT(device->vkctx, &msgCreateInfo, NULL, &device->debugMessenger) != VK_SUCCESS) {
			rgLogWarn(RG_LOG_RENDER, "VK: Debug messanger error!");
		}
	}
	else {
		rgLogWarn(RG_LOG_RENDER, "VK: No debug messanger extenion!");
	}
#endif

	Uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(device->vkctx, &deviceCount, NULL);
	if (deviceCount == 0) { RG_ERROR_MSG("No Vulkan devices found!"); }

	VkPhysicalDevice* devices = (VkPhysicalDevice*)rg_malloc(sizeof(VkPhysicalDevice) * deviceCount);
	vkEnumeratePhysicalDevices(device->vkctx, &deviceCount, devices);

	rgLogInfo(RG_LOG_RENDER, "Available devices: %d", deviceCount);

	char tbuffer[512];
	tbuffer[0] = 0;
	for (Uint32 i = 0; i < deviceCount; i++) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDevice cur_device = devices[i];
		vkGetPhysicalDeviceProperties(cur_device, &deviceProperties);

		SDL_strlcat(tbuffer, "Device[", 512);

		// Select discrete GPU ...
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			device->vkpdev = cur_device;
			//SDL_memcpy(device->cardName, deviceProperties.deviceName, SDL_strlen(deviceProperties.deviceName));
			SDL_snprintf(device->cardName, 128, "%s", deviceProperties.deviceName);
			SDL_strlcat(tbuffer, "*", 512);
		}
		else {
			SDL_strlcat(tbuffer, " ", 512);
		}
		SDL_strlcat(tbuffer, "]: ", 512);
		SDL_strlcat(tbuffer, deviceProperties.deviceName, 512);

		switch (deviceProperties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:          { SDL_strlcat(tbuffer, " Type: Other", 512); break; }
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: { SDL_strlcat(tbuffer, " Type: Integrated", 512); break; }
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   { SDL_strlcat(tbuffer, " Type: Discrete", 512); break; }
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    { SDL_strlcat(tbuffer, " Type: Vurtual", 512); break; }
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            { SDL_strlcat(tbuffer, " Type: CPU", 512); break; }
            default:                                     { SDL_strlcat(tbuffer, " Type: Unknown", 512); break; }
        }

		char vbuff[32];
		SDL_snprintf(vbuff, 32, " Vulkan: %d.%d.%d", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion));
		SDL_strlcat(tbuffer, vbuff, 512);

#if R_VKRENDER_DEBUG
		rgLogInfo(RG_LOG_RENDER, "%s", tbuffer);
#endif

	}

	// ... Or use first device
	if (!device->vkpdev) { device->vkpdev = devices[0]; }

	rg_free(devices);


	device->isAnisotropicEnabled = false;
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device->vkpdev, &supportedFeatures);
	if (supportedFeatures.samplerAnisotropy) {
		device->isAnisotropicEnabled = true;
	}


	Uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device->vkpdev, &queueFamilyCount, NULL);

	if (queueFamilyCount == 0) { RG_ERROR_MSG("No Vulkan device queues!"); }

	VkQueueFamilyProperties* fqueues = (VkQueueFamilyProperties*)rg_malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(device->vkpdev, &queueFamilyCount, fqueues);

#if 0
	rgLogInfo(RG_LOG_RENDER, " ~ ~ ~ ~ ~");
	for (Uint32 i = 0; i < queueFamilyCount; i++) {
		rgLogInfo(RG_LOG_RENDER, "Queue[%d]: R:%d C:%d T:%d SB:%d P:%d D:%d E:%d O:%d", i,
			RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_GRAPHICS_BIT),
			RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_COMPUTE_BIT),
			RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_TRANSFER_BIT),
			RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_SPARSE_BINDING_BIT),
			RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_PROTECTED_BIT),
			RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_VIDEO_DECODE_BIT_KHR),
			RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_VIDEO_ENCODE_BIT_KHR),
			RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_OPTICAL_FLOW_BIT_NV));
	}
#endif

	for (Uint32 i = 0; i < queueFamilyCount; i++) {
		if (RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_GRAPHICS_BIT) && RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_COMPUTE_BIT)) {
			device->vkqueuefamily = i;
			rgLogInfo(RG_LOG_RENDER, "Using %d queue", device->vkqueuefamily);
			break;
		}
	}
	rg_free(fqueues);

	Float32 queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = device->vkqueuefamily;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

	if (device->isAnisotropicEnabled) {
		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.samplerAnisotropy = VK_TRUE;
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
	}

	// Swapchain extension
	String extansions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = extansions;

	if (vkCreateDevice(device->vkpdev, &deviceCreateInfo, device->vkalloc, &device->vkdev) != VK_SUCCESS) {
		RG_ERROR_MSG("Vulkan device error!");
	}

	vkGetDeviceQueue(device->vkdev, device->vkqueuefamily, 0, &device->vkqueue);

	// Command pool
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = device->vkqueuefamily;
	if (vkCreateCommandPool(device->vkdev, &poolInfo, device->vkalloc, &device->vkcommandpool) != VK_SUCCESS) {
		RG_ERROR_MSG("Vulkan command pool error!");
	}

	// Descriptor pool
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4096 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  4096 },
		{ VK_DESCRIPTOR_TYPE_SAMPLER,        64 }
	};

	VkDescriptorPoolCreateInfo dpoolInfo = {};
	dpoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	dpoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	dpoolInfo.maxSets       = 4096;
	dpoolInfo.pPoolSizes    = poolSizes;
	dpoolInfo.poolSizeCount = 4;
	vkCreateDescriptorPool(device->vkdev, &dpoolInfo, device->vkalloc, &device->vkdescriptorpool);


	// Allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.vulkanApiVersion = vk_version;
	allocatorInfo.physicalDevice   = device->vkpdev;
	allocatorInfo.device           = device->vkdev;
	allocatorInfo.instance         = device->vkctx;
	if (vmaCreateAllocator(&allocatorInfo, &device->vmaallocator) != VK_SUCCESS) {
		RG_ERROR_MSG("Vulkan Memory Allocator error!");
	}

	// Swapchain

	CreateSwapchain(device);

	VkAttachmentDescription colorAttachment = {};
    colorAttachment.format         = device->vkswapchainformat.format;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment  = 0;
    colorAttachmentRef.layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass   = {};
    subpass.pipelineBindPoint      = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount   = 1;
    subpass.pColorAttachments      = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
	if (vkCreateRenderPass(device->vkdev, &renderPassInfo, device->vkalloc, &device->imguirenderpass) != VK_SUCCESS) {
		RG_ERROR_MSG("Renderpass error!");
	}

	MakeSwapchainFramebuffer(device);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.flags = 0;
	if (vkCreateSemaphore(device->vkdev, &semaphoreInfo, device->vkalloc, &device->vkpresentsemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device->vkdev, &semaphoreInfo, device->vkalloc, &device->vkeximagesemaphore) != VK_SUCCESS) {
		RG_ERROR_MSG("Present semaphore error!");
	}

	for (Uint32 i = 0; i < R_MAX_COMMANDBUFFERS_PER_FRAME; i++) {
		if (vkCreateSemaphore(device->vkdev, &semaphoreInfo, device->vkalloc, &device->cmdbuffsemaphores[i]) != VK_SUCCESS) {
			RG_ERROR_MSG("Semaphore error!");
		}
	}

	device->vkcurrentimage = 0;
	vkAcquireNextImageKHR(device->vkdev, device->vkswapchain, UINT64_MAX, device->vkeximagesemaphore, VK_NULL_HANDLE, &device->vkcurrentimage);



	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device->vkcommandpool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 2;
	vkAllocateCommandBuffers(device->vkdev, &allocInfo, device->vkswapcmdbuffer);

	vkResetCommandBuffer(device->vkswapcmdbuffer[0], 0);
	VkCommandBufferBeginInfo begininfo = {};
	begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(device->vkswapcmdbuffer[0], &begininfo);

	for (Uint32 i = 0; i < device->vkimagescount; i++) {
		// Transition image layout to COLOR_ATTACHMENT
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = device->vkswapimages[i];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(device->vkswapcmdbuffer[0],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, NULL,
			0, NULL,
			1, &barrier);
	}

	vkEndCommandBuffer(device->vkswapcmdbuffer[0]);

	VkPipelineStageFlags f[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &device->vkswapcmdbuffer[0];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &device->vkeximagesemaphore;
	submitInfo.pWaitDstStageMask = f;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &device->cmdbuffsemaphores[0];
	vkQueueSubmit(device->vkqueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(device->vkqueue);
	device->cmdsemaphore = 0;

	SDL_SetWindowPosition(device->hwnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	rgLogInfo(RG_LOG_RENDER, "Initialized Vulkan rendering backend");
	return device;
}

void R_DestroyDevice(RRenderDevice* device) {
	STDAllocator* alloc = (STDAllocator*)device->allocator;

	Engine::FreeEventHandler(_EventHandler);

	vkQueueWaitIdle(device->vkqueue);
	vkDeviceWaitIdle(device->vkdev);

	vkFreeCommandBuffers(device->vkdev, device->vkcommandpool, 2, device->vkswapcmdbuffer);
	vkDestroySemaphore(device->vkdev, device->vkeximagesemaphore, device->vkalloc);
	vkDestroySemaphore(device->vkdev, device->vkpresentsemaphore, device->vkalloc);
	for (Uint32 i = 0; i < R_MAX_COMMANDBUFFERS_PER_FRAME; i++) {
		vkDestroySemaphore(device->vkdev, device->cmdbuffsemaphores[i], device->vkalloc);
	}

	DestroySwapchain(device);
	vkDestroyRenderPass(device->vkdev, device->imguirenderpass, device->vkalloc);

	vkDestroyCommandPool(device->vkdev, device->vkcommandpool, device->vkalloc);
	vkDestroyDescriptorPool(device->vkdev, device->vkdescriptorpool, device->vkalloc);

	vmaDestroyAllocator(device->vmaallocator);

	vkDestroyDevice(device->vkdev, device->vkalloc);

#if R_VKRENDER_DEBUG
	auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(device->vkctx, "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXT != NULL) {
		vkDestroyDebugUtilsMessengerEXT(device->vkctx, device->debugMessenger, NULL);
	}
#endif

	vkDestroyInstance(device->vkctx, device->vkalloc);

	// Free device object
	RG_DELETE_CLASS(alloc, RRenderDevice, device);
	// And delete allocator
	RG_DELETE(STDAllocator, alloc);
}

void R_SwapBuffers(RRenderDevice* device, RSwapBuffersInfo* info) {

	// Wait render end
	//vkDeviceWaitIdle(device->vkdev);

	// Submit an empty command buffer

	vkResetCommandBuffer(device->vkswapcmdbuffer[0], 0);
	VkCommandBufferBeginInfo begininfo = {};
	begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(device->vkswapcmdbuffer[0], &begininfo);
#if 1
	{
		// Transition image layout to PRESENT
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = device->vkswapimages[device->vkcurrentimage];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(device->vkswapcmdbuffer[0],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, NULL,
			0, NULL,
			1, &barrier);
	}
#endif
	vkEndCommandBuffer(device->vkswapcmdbuffer[0]);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &device->vkswapcmdbuffer[0];

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores    = &device->cmdbuffsemaphores[device->cmdsemaphore];

	VkPipelineStageFlags f[] = {
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	};
	submitInfo.pWaitDstStageMask = f;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &device->vkpresentsemaphore;
	vkQueueSubmit(device->vkqueue, 1, &submitInfo, NULL);
	//vkQueueWaitIdle(device->vkqueue);

	VkSwapchainKHR swapchain[] = { device->vkswapchain };

	VkPresentInfoKHR pinfo = {};
	pinfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pinfo.waitSemaphoreCount = 1;
	pinfo.pWaitSemaphores    = &device->vkpresentsemaphore;
	pinfo.swapchainCount     = 1;
	pinfo.pSwapchains        = swapchain;
	pinfo.pImageIndices      = &device->vkcurrentimage;
	pinfo.pResults           = NULL;

	VkResult result;
	if ((result = vkQueuePresentKHR(device->vkqueue, &pinfo)) != VK_SUCCESS) {
		rgLogError(RG_LOG_RENDER, "Queue: %d", result);
		//RG_ERROR_MSG("Queue error!");
	}

	//vkDeviceWaitIdle(device->vkdev);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || RG_CHECK_FLAG(info->flags, RG_SWAPCHAIN_FLAG_RESIZE)) {
		// Resize requested!
		ResizeSwapchain(device);
	}

	device->vkcurrentimage++;
	device->vkcurrentimage = device->vkcurrentimage % device->vkimagescount;

	vkAcquireNextImageKHR(device->vkdev, device->vkswapchain, UINT64_MAX, device->vkeximagesemaphore, VK_NULL_HANDLE, &device->vkcurrentimage);

	// Wait next image

	vkResetCommandBuffer(device->vkswapcmdbuffer[1], 0);
	begininfo = {};
	begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(device->vkswapcmdbuffer[1], &begininfo);

	{
		// Transition image layout to COLOR_ATTACHMENT
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = device->vkswapimages[device->vkcurrentimage];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(device->vkswapcmdbuffer[1],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0, NULL,
			0, NULL,
			1, &barrier);
	}

	vkEndCommandBuffer(device->vkswapcmdbuffer[1]);
	submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &device->vkswapcmdbuffer[1];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &device->vkeximagesemaphore;
	submitInfo.pWaitDstStageMask = f;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &device->cmdbuffsemaphores[0];

	vkQueueSubmit(device->vkqueue, 1, &submitInfo, NULL);
	vkQueueWaitIdle(device->vkqueue);

	// Reset
	device->cmdsemaphore = 0;

	device->draw_calls = 0;
	device->dispatch_calls = 0;
}

void R_GetInfo(RRenderDevice* dev, RenderInfo* info) {

	VmaBudget budgets[16] = {};
	vmaGetHeapBudgets(dev->vmaallocator, budgets);

	info->buffers_memory  = dev->buffersMemLen;
	info->textures_memory = dev->imageMemLen;

	info->r3d_draw_calls = dev->draw_calls;
	info->r3d_dispatch_calls = dev->dispatch_calls;

	info->dedicated_memory = budgets[0].usage + budgets[0].budget;
	info->shared_memory = 0;
	for (Uint32 i = 1; i < 16; i++) {
		info->shared_memory += budgets[i].usage + budgets[i].budget;
	}
	
	info->renderer = dev->cardName;
	info->render_name = "Vulkan";
}

void R_ImGui_Init(RRenderDevice* dev) {

	ImGui_ImplVulkan_InitInfo info = {};
	info.ApiVersion     = vk_version;
	info.Instance       = dev->vkctx;
	info.PhysicalDevice = dev->vkpdev;
	info.Device         = dev->vkdev;
	info.QueueFamily    = dev->vkqueuefamily;
	info.Queue          = dev->vkqueue;
	info.DescriptorPoolSize = 128;
	info.RenderPass     = dev->imguirenderpass;
	info.MinImageCount  = dev->vkimagescount;
	info.ImageCount     = dev->vkimagescount;
	info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&info);
}

void R_ImGui_Shutdown(RRenderDevice* dev) {
	ImGui_ImplVulkan_Shutdown();
}

void R_ImGui_NewFrame(RRenderDevice* dev) {
	ImGui_ImplVulkan_NewFrame();
}