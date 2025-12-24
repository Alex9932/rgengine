#ifndef _SWAPCHAIN_H
#define _SWAPCHAIN_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include "rendertypesvk.h"

void CreateSwapchain(RRenderDevice* dev);
void MakeSwapchainFramebuffer(RRenderDevice* dev);
void DestroySwapchain(RRenderDevice* dev);
void ResizeSwapchain(RRenderDevice* dev);

#endif