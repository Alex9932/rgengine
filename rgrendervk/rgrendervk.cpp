#define DLL_EXPORT
#include "rgrendervk.h"

#include <rshared.h>
#include <rgvector.h>
#include <allocator.h>
#include <engine.h>
#include <event.h>
#include <render.h>

#include "vk.h"
#include "swapchain.h"
#include "imgui_impl_vulkan.h"

#include <vulkan/vulkan.h>

static SDL_Window*        hwnd         = NULL;
static Engine::Allocator* allocator    = NULL;

static VkCommandBuffer    imgui_buffer = NULL;

static bool _EventHandler(SDL_Event* event) {

	if (event->type == Engine::GetUserEventID()) {

		switch (event->user.code) {
			case RG_EVENT_RENDER_VIEWPORT_RESIZE: {
				// Resize swapchain
				vec2* wndSize = (vec2*)event->user.data1;
				rgLogWarn(RG_LOG_RENDER, "Size changed: %dx%d", (Uint32)wndSize->x, (Uint32)wndSize->y);
				break;
			}
			default: { break; }
		}

	}

	return true;
}

Engine::Allocator* RGetAllocator() {
	return allocator;
}

// PUBLIC API

SDL_Window* R_ShowWindow(Uint32 w, Uint32 h) {
	return SDL_CreateWindow("rgEngine", 5, 5, w, h, SDL_WINDOW_VULKAN);
}

void R_Setup(RenderSetupInfo* info) {
	allocator = new Engine::STDAllocator("Vulkan allocator");
}

void R_Initialize(SDL_Window* wnd) {
	RG_ASSERT_MSG(wnd, "Invalid window pointer!");
	hwnd = wnd;

	SDL_SetWindowPosition(hwnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowTitle(hwnd, "rgEngine - Vulkan");

	VK_Initialize(hwnd);
	
	Engine::RegisterEventHandler(_EventHandler);

	imgui_buffer = VK_AllocateCommandBuffer(VK_GetCommandPool());

	ImGui_ImplVulkan_InitInfo info = {};
	info.ApiVersion     = VK_GetVersion();
	info.Instance       = VK_GetInstance();
	info.PhysicalDevice = VK_GetPhysicalDevice();
	info.Device         = VK_GetDevice();
	info.QueueFamily    = VK_GetQueueFamily();
	info.Queue          = VK_GetQueue();
	info.DescriptorPoolSize = 128;
	info.RenderPass     = VK_GetRenderpass();
	info.MinImageCount  = GetSwapchainImageCount();
	info.ImageCount     = GetSwapchainImageCount();
	info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&info);
	ImGui_ImplVulkan_NewFrame();

	rgLogInfo(RG_LOG_RENDER, "Vulkan: %s", VK_GetGraphicsCardName());
	rgLogInfo(RG_LOG_RENDER, "Initialized Vulkan rendering backend");

	//RG_ERROR_MSG("VULKAN RENDERER IS NOT IMPLEMENTED YET");
}

void R_Destroy() {
	rgLogInfo(RG_LOG_RENDER, "Destroy renderer");

	ImGui_ImplVulkan_Shutdown();

	VK_FreeCommandBuffer(VK_GetCommandPool(), imgui_buffer);

	Engine::FreeEventHandler(_EventHandler);
	delete allocator;

}

void R_SwapBuffers() {

	// Wait main render task

	// Draw rendered buffers to screen

	// Draw ImGui
	vkResetCommandBuffer(imgui_buffer, 0);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = NULL;
	vkBeginCommandBuffer(imgui_buffer, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = VK_GetRenderpass();
	renderPassInfo.framebuffer = GetSwapchainFramebuffer(VK_GetCurrentImageIdx());
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = GetSwapchainExtent();
	VkClearValue clearColor = { {{1.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(imgui_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imgui_buffer, NULL);

	vkCmdEndRenderPass(imgui_buffer);
	vkEndCommandBuffer(imgui_buffer);

	VkSemaphore imagesemaphore = VK_GetImageAvailableSemaphore();
	VkSemaphore presentsemaphore = VK_GetPresentSemaphore();
	VkSubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &imgui_buffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imagesemaphore;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &presentsemaphore;
	vkQueueSubmit(VK_GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);

	// Present
	VK_Present();

	// Load textures

	// Resize buffers (if needed)
}

void R_GetInfo(RenderInfo* info) {

#if 0

	R3DStats r3d_stats = {};
	GetR3DStats(&r3d_stats);

#endif

	info->render_name        = "Vulkan Renderer";
	info->renderer           = VK_GetGraphicsCardName();

#if 0

	info->shared_memory      = 0;
	info->dedicated_memory   = 0;

	info->textures_memory    = GetTextureMemory();
	info->buffers_memory     = GetBufferMemory();

	info->textures_left      = TexturesInQueue();
	info->textures_inQueue   = TexturesLeft();
	info->textures_loaded    = r3d_stats.texturesLoaded;

	info->meshes_loaded      = r3d_stats.modelsLoaded;

	info->r3d_draw_calls     = r3d_stats.drawCalls;
	info->r3d_dispatch_calls = r3d_stats.dispatchCalls;

	/*if (RG_CHECK_FLAG(flags, RG_RENDER_NOLIGHT)) {
		info->r3d_renderResult = GetGBufferShaderResource(0);
	} else*/ if (RG_CHECK_FLAG(flags, RG_RENDER_NOPOSTPROCESS)) {
		info->r3d_renderResult = GetLightpassShaderResource();
	}
	else {
		info->r3d_renderResult = FXGetOuputTexture();
	}

	info->r2d_renderResult = R2DGetOuputTexture();

#endif

}