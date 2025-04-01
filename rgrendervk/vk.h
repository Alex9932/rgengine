#ifndef _VK_H
#define _VK_H

#include "rgtypes.h"

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

void VK_Initialize(SDL_Window* hwnd);
void VK_Destroy();
void VK_Present();

VkCommandBuffer VK_AllocateCommandBuffer(VkCommandPool pool);
void VK_FreeCommandBuffer(VkCommandPool pool, VkCommandBuffer buffer);

String VK_GetGraphicsCardName();

Uint32 VK_GetVersion();
VkInstance VK_GetInstance();
VkPhysicalDevice VK_GetPhysicalDevice();
VkDevice VK_GetDevice();
Uint32 VK_GetQueueFamily();
VkQueue VK_GetQueue();
VkRenderPass VK_GetRenderpass();
VkCommandPool VK_GetCommandPool();
Uint32 VK_GetCurrentImageIdx();
VkSemaphore VK_GetPresentSemaphore();
VkSemaphore VK_GetImageAvailableSemaphore();

#endif