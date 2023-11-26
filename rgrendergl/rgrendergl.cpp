#define DLL_EXPORT
#include "rgrendergl.h"

#include <rshared.h>
#include <rgvector.h>
#include <allocator.h>
#include <engine.h>
#include <event.h>
#include <render.h>

#include <GL/GL.h>

static SDL_Window*        hwnd      = NULL;
static SDL_GLContext      glctx;
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
	return SDL_CreateWindow("rgEngine", 5, 5, w, h, SDL_WINDOW_OPENGL);
}

void R_Setup(RenderSetupInfo* info) {
	allocator = new Engine::STDAllocator("OpenGL allocator");
}

void R_Initialize(SDL_Window* wnd) {
	RG_ASSERT_MSG(wnd, "Invalid window pointer!");
	hwnd = wnd;

	SDL_SetWindowPosition(hwnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowTitle(hwnd, "rgEngine - OpenGL");

	//DX11_Initialize(hwnd);
	Engine::RegisterEventHandler(_EventHandler);

	ivec2 size = {};
	SDL_GetWindowSize(hwnd, &size.x, &size.y);

	glctx = SDL_GL_CreateContext(hwnd);
	SDL_GL_SetSwapInterval(0);

	rgLogInfo(RG_LOG_RENDER, "OpenGL: %s", glGetString(GL_RENDERER));
	rgLogInfo(RG_LOG_RENDER, "Initialized OpenGL rendering backend");
}

void R_Destroy() {
	rgLogInfo(RG_LOG_RENDER, "Destroy renderer");

#if 0
	DestroyR3D();

	// TEMP
	delete vBuffer;
	delete shader;

	DX11_Destroy();
#endif

	Engine::FreeEventHandler(_EventHandler);
	delete allocator;

	SDL_GL_DeleteContext(glctx);
}

void R_SwapBuffers() {

	vec4 clearColor = { 0, 0, 1, 1 };
	
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT);

	SDL_GL_SwapWindow(hwnd);
}