#include "vk.h"

#include <engine.h>
#include <allocator.h>

#include <vulkan/vulkan.h>

static VkInstance instance;

void VK_Initialize() {
	Uint32 instanceLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
	rgLogInfo(RG_LOG_RENDER, "Vulkan layers: %d", instanceLayerCount);

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Vulkan GPU Info";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "rgEngine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        RG_ERROR_MSG("Vulkan instance error!");
    }

    Uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    VkPhysicalDevice* devices = (VkPhysicalDevice*)rg_malloc(sizeof(VkPhysicalDevice) * deviceCount);

    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    for (Uint32 i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDevice device = devices[i];
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        rgLogInfo(RG_LOG_RENDER, "--==========================================--");
        rgLogInfo(RG_LOG_RENDER, "Device: %s", deviceProperties.deviceName);

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

    rg_free(devices);

}

void VK_Destroy() {
    vkDestroyInstance(instance, nullptr);
}