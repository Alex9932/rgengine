#define DLL_EXPORT
#include "rgrenderdx11.h"

#include <rshared.h>
#include <rgvector.h>
#include <engine.h>
#include <render.h>
#include <window.h>

#include <event.h>
#include <input.h>

#include "dx11.h"
#include "r3d.h"
#include "shader.h"

#include "gbuffer.h"
#include "lightpass.h"

#include "loader.h"

#include <rgmath.h>
#include <filesystem.h>

#include "postprocess.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_impl_dx11.h"

//shadersdx11

static SDL_Window*        hwnd           = NULL;
static Engine::Allocator* allocator      = NULL;

static ivec2              wndSize        = {0};
static Bool               wndResized     = false;

static RenderFlags        flags;

static bool _EventHandler(SDL_Event* event) {

	if (event->type == Engine::GetUserEventID()) {

		switch (event->user.code) {
			case RG_EVENT_RENDER_VIEWPORT_RESIZE: {
				// Resize swapchain
				ivec2* wnd_size = (ivec2*)event->user.data1;
				wndSize    = *wnd_size;
				wndResized = true;
				rgLogWarn(RG_LOG_RENDER, "Size changed: %dx%d", wnd_size->x, wnd_size->y);
				break;
			}
			default: { break; }
		}

	}

	if (event->type == SDL_KEYDOWN) {
		switch (event->key.keysym.scancode) {
			case SDL_SCANCODE_R: {
				if (Engine::IsKeyDown(SDL_SCANCODE_LSHIFT) && Engine::IsKeyDown(SDL_SCANCODE_LALT)) {
					// Reload shaders
					rgLogInfo(RG_LOG_RENDER, "Reloading shaders...");

					ReloadShadersR3D();

				}
				break;
			}
			case SDL_SCANCODE_1: { SetViewIDX(0); break; }
			case SDL_SCANCODE_2: { SetViewIDX(1); break; }
			case SDL_SCANCODE_3: { SetViewIDX(2); break; }
			case SDL_SCANCODE_4: { SetViewIDX(3); break; }
			case SDL_SCANCODE_5: { SetViewIDX(4); break; }
			case SDL_SCANCODE_6: { SetViewIDX(5); break; }
			case SDL_SCANCODE_7: { SetViewIDX(6); break; }
			case SDL_SCANCODE_8: { SetViewIDX(7); break; }
			default: { break; }
		}
	}

	return true;
}

Engine::Allocator* RGetAllocator() {
	return allocator;
}

// NOT TEMPORARY DATA //

static Float32 quad_data[] = {
	-1,  1,  1,  1,  1, -1,
	 1, -1, -1, -1, -1,  1
};

static Shader* shader = NULL;
static Buffer* vBuffer = NULL;

////////////////////

// PUBLIC API

SDL_Window* R_ShowWindow(Uint32 w, Uint32 h) {
	wndSize.x = w;
	wndSize.y = h;
	return SDL_CreateWindow("rgEngine", 5, 5, w, h, SDL_WINDOW_SHOWN);
}

void R_Setup(RenderSetupInfo* info) {
	allocator = new Engine::STDAllocator("DX11 allocator");
	//Engine::RegisterAllocator(allocator);

	flags = info->flags;

}

void R_Initialize(SDL_Window* wnd) {
	RG_ASSERT_MSG(wnd, "Invalid window pointer!");
	hwnd = wnd;

	SDL_SetWindowPosition(hwnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowTitle(hwnd, "rgEngine - D3D11");

	DX11_Initialize(hwnd);
	
	Engine::RegisterEventHandler(_EventHandler);

	ivec2 size = {};
	SDL_GetWindowSize(hwnd, &size.x, &size.y);

	// ImGui
	ImGui_ImplDX11_Init(DX11_GetDevice(), DX11_GetContext());
	ImGui_ImplDX11_NewFrame();
	//ImGui::SetCurrentContext(Engine::GetImGuiContext());

	LoaderInit();

	InitializeR3D(&size);

	// TEMP
	InputDescription staticDescriptions[1] = {};
	staticDescriptions[0].name      = "POSITION";
	staticDescriptions[0].inputSlot = 0;
	staticDescriptions[0].format    = INPUT_R32G32_FLOAT;
	SamplerDescription sampler = {};
	sampler.u = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.v = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler.w = D3D11_TEXTURE_ADDRESS_CLAMP;
	PipelineDescription desc = {};
	desc.inputCount   = 1;
	desc.descriptions = staticDescriptions;
	desc.sampler	  = &sampler;

	char bp2d_vs[128];
	char bp2d_ps[128];
	Engine::GetPath(bp2d_vs, 128, RG_PATH_SYSTEM, "shadersdx11/bypass2d.vs");
	Engine::GetPath(bp2d_ps, 128, RG_PATH_SYSTEM, "shadersdx11/bypass2d.ps");
	shader = RG_NEW_CLASS(allocator, Shader)(&desc, bp2d_vs, bp2d_ps, false);

	BufferCreateInfo vbufferInfo = {};
	vbufferInfo.type   = BUFFER_VERTEX;
	vbufferInfo.access = BUFFER_GPU_ONLY;
	vbufferInfo.usage  = BUFFER_DEFAULT;
	vbufferInfo.length = sizeof(quad_data);
	vBuffer = RG_NEW_CLASS(allocator, Buffer)(&vbufferInfo);
	vBuffer->SetData(0, vbufferInfo.length, quad_data);

	rgLogInfo(RG_LOG_RENDER, "Initialized D3D11 rendering backend");
}

void R_Destroy() {
	rgLogInfo(RG_LOG_RENDER, "Destroy renderer");

	ImGui_ImplDX11_Shutdown();
	DestroyR3D();
	// TEMP
	RG_DELETE_CLASS(allocator, Buffer, vBuffer);
	RG_DELETE_CLASS(allocator, Shader, shader);

	LoaderDestroy();

	DX11_Destroy();
	Engine::FreeEventHandler(_EventHandler);
	delete allocator;
	//Engine::FreeAllocator(allocator);
}

void R_SwapBuffers() {
	DX11_SetViewport(wndSize.x, wndSize.y);

    vec4 clearColor = { 0, 0, 0, 1 };

	DX11_BindDefaultFramebuffer();
	DX11_Clear((Float32*)&clearColor);

	if (RG_CHECK_FLAG(flags, RG_RENDER_FULLSCREEN)) {
		shader->Bind();

		//ID3D11ShaderResourceView* res0 = GetGBufferShaderResource(0);
		//ID3D11ShaderResourceView* res1 = GetLightpassShaderResource();
		ID3D11ShaderResourceView* res0 = FXGetOuputTexture();

		DX11_GetContext()->PSSetShaderResources(0, 1, &res0);
		//DX11_GetContext()->PSSetShaderResources(1, 1, &res1);

		UINT stride = sizeof(Float32) * 2;
		UINT offset = 0;
		ID3D11Buffer* vbuffer = vBuffer->GetHandle();
		DX11_GetContext()->IASetVertexBuffers(0, 1, &vbuffer, &stride, &offset);
		//DX11_GetContext()->IASetIndexBuffer(mdl->iBuffer->GetHandle(), GetIndexType(mdl->iType), 0);
		DX11_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		DX11_GetContext()->Draw(6, 0);
	}

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	ImGui_ImplDX11_NewFrame();

	DX11_SwapBuffers();

	DoLoadTextures();

	if (wndResized) {
		wndResized = false;
		DX11_Resize(&wndSize);
		ResizeR3D(&wndSize);
		rgLogInfo(RG_LOG_RENDER, "Swapchain resized!");
	}
}

void R_GetInfo(RenderInfo* info) {

	R3DStats r3d_stats = {};
	GetR3DStats(&r3d_stats);

	info->render_name        = "Direct3D 11 Renderer";
	info->renderer           = DX11_GetGraphicsCardName();

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

	info->r3d_renderResult   = FXGetOuputTexture();

}