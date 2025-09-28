#ifndef _SWAPCHAIN_H
#define _SWAPCHAIN_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

void CreateSwapchain(SDL_Window* hwnd);
void MakeSwapchainFramebuffer();
void DestroySwapchain();
void ResizeSwapchain();

Uint32 GetSwapchainImageCount();
VkFramebuffer GetSwapchainFramebuffer(Uint32 idx);
VkExtent2D GetSwapchainExtent();
VkSwapchainKHR GetSwapchain();
VkSurfaceFormatKHR GetSwapchainFormat();

#endif