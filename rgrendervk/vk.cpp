#include "vk.h"

#include <engine.h>
#include <allocator.h>

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "swapchain.h"

static Uint32           vk_version          = VK_API_VERSION_1_3;
static VkInstance       vk_instance         = NULL;
static VkPhysicalDevice vk_phys_device      = NULL;
static VkDevice         vk_device           = NULL;
static Uint32           vk_queuefamily      = 0;
static VkQueue          vk_queue            = NULL;

static VkCommandPool    vk_commandpool      = NULL;
static VkSemaphore      vk_eximagesemaphore = NULL; // Image available
static VkSemaphore      vk_presentsemaphore = NULL;

// Present
static VkRenderPass     vk_renderpass       = NULL;
static Uint32           vk_currentimage     = 0;

static char vk_cardname[128] = { 0 };

void VK_Initialize(SDL_Window* hwnd) {
	Uint32 instanceLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    VkLayerProperties* layers = (VkLayerProperties*)rg_malloc(sizeof(VkLayerProperties) * instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, layers);
    
	rgLogInfo(RG_LOG_RENDER, "Vulkan layers: %d", instanceLayerCount);
    for (Uint32 i = 0; i < instanceLayerCount; i++) {
        rgLogInfo(RG_LOG_RENDER, "-> %s", layers[i].layerName);
    }
    rg_free(layers);

    Uint32 sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(nullptr, &sdlExtensionCount, nullptr);
    String* sdlExtensions = (String*)rg_malloc(sizeof(String) * sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &sdlExtensionCount, sdlExtensions);

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Vulkan GPU Info";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "rgEngine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = vk_version;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = sdlExtensionCount;
    createInfo.ppEnabledExtensionNames = sdlExtensions;

    if (vkCreateInstance(&createInfo, nullptr, &vk_instance) != VK_SUCCESS) {
        RG_ERROR_MSG("Vulkan instance error!");
    }

    rg_free(sdlExtensions);

    /////////////////////////////////////////////
    // Physical device

    Uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(vk_instance, &deviceCount, nullptr);

    if (deviceCount == 0) { RG_ERROR_MSG("No Vulkan devices found!"); }

    VkPhysicalDevice* devices = (VkPhysicalDevice*)rg_malloc(sizeof(VkPhysicalDevice) * deviceCount);

    vkEnumeratePhysicalDevices(vk_instance, &deviceCount, devices);

    rgLogInfo(RG_LOG_RENDER, "Available devices: %d", deviceCount);
    for (Uint32 i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDevice cur_device = devices[i];
        vkGetPhysicalDeviceProperties(cur_device, &deviceProperties);

        rgLogInfo(RG_LOG_RENDER, " ~ ~ ~ ~ ~");
        rgLogInfo(RG_LOG_RENDER, "Device: %s", deviceProperties.deviceName);

        // Select discrete GPU ...
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            vk_phys_device = cur_device;
            SDL_memcpy(vk_cardname, deviceProperties.deviceName, SDL_strlen(deviceProperties.deviceName));
        }

        switch (deviceProperties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:          { rgLogInfo(RG_LOG_RENDER, "Type: Other"); break; }
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: { rgLogInfo(RG_LOG_RENDER, "Type: Integrated"); break; }
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   { rgLogInfo(RG_LOG_RENDER, "Type: Discrete"); break; }
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    { rgLogInfo(RG_LOG_RENDER, "Type: Vurtual"); break; }
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            { rgLogInfo(RG_LOG_RENDER, "Type: CPU"); break; }
            default:                                     { rgLogInfo(RG_LOG_RENDER, "Type: Unknown"); break; }
        }

        rgLogInfo(RG_LOG_RENDER, "Vulkan: %d.%d.%d", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion));

    }

    // ... Or use first device
    if (!vk_phys_device) { vk_phys_device = devices[0]; }

    rg_free(devices);

    /////////////////////////////////////////////
    // Device & Queue family

    Uint32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_phys_device, &queueFamilyCount, nullptr);

    if (queueFamilyCount == 0) { RG_ERROR_MSG("No Vulkan device queues!"); }

    VkQueueFamilyProperties* fqueues = (VkQueueFamilyProperties*)rg_malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(vk_phys_device, &queueFamilyCount, fqueues);

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

    /////////////////////////////////////////////
    // Select Graphics & Compute family

    for (Uint32 i = 0; i < queueFamilyCount; i++) {
        if (RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_GRAPHICS_BIT) && RG_CHECK_FLAG(fqueues[i].queueFlags, VK_QUEUE_COMPUTE_BIT)) {
            vk_queuefamily = i;
            rgLogInfo(RG_LOG_RENDER, "Using %d queue", vk_queuefamily);
            break;
        }
    }

    rg_free(fqueues);

    Float32 queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = vk_queuefamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    
    // Swapchain extension
    String extansions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    deviceCreateInfo.enabledExtensionCount   = 1;
    deviceCreateInfo.ppEnabledExtensionNames = extansions;

    if (vkCreateDevice(vk_phys_device, &deviceCreateInfo, nullptr, &vk_device) != VK_SUCCESS) {
        RG_ERROR_MSG("Vulkan device error!");
    }

    vkGetDeviceQueue(vk_device, vk_queuefamily, 0, &vk_queue);

    /////////////////////////////////////////////
    // Create swapchain

    CreateSwapchain(hwnd);

    /////////////////////////////////////////////
    // Renderpass for draw to swapchain image

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format         = GetSwapchainFormat().format;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    if (vkCreateRenderPass(vk_device, &renderPassInfo, nullptr, &vk_renderpass) != VK_SUCCESS) {
        RG_ERROR_MSG("Renderpass error!");
    }

    MakeSwapchainFramebuffer();

    /////////////////////////////////////////////
    // CommandPool

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vk_queuefamily;
    if (vkCreateCommandPool(vk_device, &poolInfo, nullptr, &vk_commandpool) != VK_SUCCESS) {
        RG_ERROR_MSG("CommandPool error!");
    }

    /////////////////////////////////////////////
    // Present semaphore

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.flags = 0;
    if (vkCreateSemaphore(vk_device, &semaphoreInfo, nullptr, &vk_presentsemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vk_device, &semaphoreInfo, nullptr, &vk_eximagesemaphore) != VK_SUCCESS) {
        RG_ERROR_MSG("Present semaphore error!");
    }

    vkAcquireNextImageKHR(vk_device, GetSwapchain(), UINT64_MAX, vk_eximagesemaphore, VK_NULL_HANDLE, &vk_currentimage);
}

void VK_Destroy() {
    DestroySwapchain();
    vkDestroySemaphore(vk_device, vk_eximagesemaphore, nullptr);
    vkDestroySemaphore(vk_device, vk_presentsemaphore, nullptr);
    vkDestroyRenderPass(vk_device, vk_renderpass, nullptr);
    vkDestroyCommandPool(vk_device, vk_commandpool, nullptr);
    vkDestroyDevice(vk_device, nullptr);
    vkDestroyInstance(vk_instance, nullptr);
}

void VK_Present() {

    //vkDeviceWaitIdle(vk_device);

    VkSwapchainKHR swapchain[] = { GetSwapchain() };

    VkPresentInfoKHR info = {};
    info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores    = &vk_presentsemaphore;

    info.swapchainCount     = 1;
    info.pSwapchains        = swapchain;

    info.pImageIndices      = &vk_currentimage;

    info.pResults = NULL;

    VkResult result;
    if ((result = vkQueuePresentKHR(vk_queue, &info)) != VK_SUCCESS) {
        rgLogError(RG_LOG_RENDER, "Queue: %d", result);
        //RG_ERROR_MSG("Queue error!");
    }

    vkDeviceWaitIdle(vk_device);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Resize requested!
        ResizeSwapchain();
    }

    vk_currentimage++;
    vk_currentimage = vk_currentimage % GetSwapchainImageCount();
    //rgLogError(RG_LOG_RENDER, "Image: %d", vk_currentimage);
    vkAcquireNextImageKHR(vk_device, GetSwapchain(), UINT64_MAX, vk_eximagesemaphore, VK_NULL_HANDLE, &vk_currentimage);
}

VkCommandBuffer VK_AllocateCommandBuffer(VkCommandPool pool) {
    VkCommandBufferAllocateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = pool;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vk_device, &info, &cmd);
    return cmd;
}

void VK_FreeCommandBuffer(VkCommandPool pool, VkCommandBuffer buffer) {
    vkFreeCommandBuffers(vk_device, pool, 1, &buffer);
}

String VK_GetGraphicsCardName() { return vk_cardname; }

Uint32           VK_GetVersion()          { return vk_version; }
VkInstance       VK_GetInstance()         { return vk_instance; }
VkPhysicalDevice VK_GetPhysicalDevice()   { return vk_phys_device; }
VkDevice         VK_GetDevice()           { return vk_device; }
Uint32           VK_GetQueueFamily()      { return vk_queuefamily; }
VkQueue          VK_GetQueue()            { return vk_queue; }
VkRenderPass     VK_GetRenderpass()       { return vk_renderpass; }
VkCommandPool    VK_GetCommandPool()      { return vk_commandpool; }
Uint32           VK_GetCurrentImageIdx()  { return vk_currentimage; }
VkSemaphore      VK_GetPresentSemaphore() { return vk_presentsemaphore; }
VkSemaphore      VK_GetImageAvailableSemaphore() { return vk_eximagesemaphore; }