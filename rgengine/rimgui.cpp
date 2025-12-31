#define DLL_EXPORT
#include "rimgui.h"

#include "render.h"
#include "guiwnds.h"

#include <vector>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"

namespace Engine {
	namespace Render {

        static std::vector<RenderImGuiCallback> imguicallbacks;

        static RCommandBuffer* cmdbuffer = NULL;

        void InitRImGui() {
            RRenderDevice* dev = GetRenderDevice();
            RenderBackend* ctx = GetRenderContext();

            RCommandBufferCreateInfo cmdbuffinfo = {};
            cmdbuffinfo.maxcmds = 128;
            cmdbuffer = ctx->CreateCommandBuffer(dev, &cmdbuffinfo);

            ctx->ImGui_Init(dev);
        }

        void DestroyRImGui() {
            RRenderDevice* dev = GetRenderDevice();
            RenderBackend* ctx = GetRenderContext();
            ctx->DestroyCommandBuffer(cmdbuffer);
            ctx->ImGui_Shutdown(dev);
        }

		void UpdateImGui() {
            RRenderDevice* dev = GetRenderDevice();
            RenderBackend* ctx = GetRenderContext();

            // Update ImGui
            ctx->ImGui_NewFrame(dev);
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // Call all registered callbacks
            std::vector<RenderImGuiCallback>::iterator it;
            for (it = imguicallbacks.begin(); it != imguicallbacks.end(); it++) {
                RenderImGuiCallback cb = *it;
                cb();
            }

            DrawRendererStats();

            ImGui::EndFrame();
            ImGui::Render();
		}

        void DrawImGui() {
            RenderBackend* ctx = GetRenderContext();

            // Draw imgui
            ctx->ResetCommandBuffer(cmdbuffer);
            ctx->BeginCommandBuffer(cmdbuffer);

            ctx->CmdBeginRenderpass(cmdbuffer, NULL);
            ctx->CmdImGuiRenderDrawData(cmdbuffer, ImGui::GetDrawData());
            ctx->CmdEndRenderpass(cmdbuffer);

            ctx->EndCommandBuffer(cmdbuffer);

            RCommandBufferSubmitInfo submitinfo = {};
            submitinfo.buffer = cmdbuffer;
            ctx->SubmitCommandBuffer(&submitinfo);
        }

        void RegisterImGuiDrawCallback(RenderImGuiCallback cb) {
            imguicallbacks.push_back(cb);
        }

        void FreeImGuiDrawCallback(RenderImGuiCallback cb) {
            std::vector<RenderImGuiCallback>::iterator it;
            for (it = imguicallbacks.begin(); it != imguicallbacks.end(); it++) {
                if (*it == cb) {
                    *it = std::move(imguicallbacks.back());
                    imguicallbacks.pop_back();
                    break;
                }
            }
        }

	}
}