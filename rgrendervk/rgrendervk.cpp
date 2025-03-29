#define DLL_EXPORT
#include "rgrendervk.h"

#include <rshared.h>
#include <rgvector.h>
#include <allocator.h>
#include <engine.h>
#include <event.h>
#include <render.h>

#include "vk.h"

static SDL_Window*        hwnd      = NULL;
static Engine::Allocator* allocator = NULL;

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

	VK_Initialize();
	
	Engine::RegisterEventHandler(_EventHandler);

	//rgLogInfo(RG_LOG_RENDER, "OpenGL: %s", glGetString(GL_RENDERER));
	rgLogInfo(RG_LOG_RENDER, "Initialized Vulkan rendering backend");

	RG_ERROR_MSG("VULKAN RENDERER IS NOT IMPLEMENTED YET");
}

void R_Destroy() {
	rgLogInfo(RG_LOG_RENDER, "Destroy renderer");



	Engine::FreeEventHandler(_EventHandler);
	delete allocator;

}

void R_SwapBuffers() {


}

void R_GetInfo(RenderInfo* info) {

}