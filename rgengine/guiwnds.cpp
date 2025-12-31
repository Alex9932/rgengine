#define DLL_EXPORT
#include "render.h"
#include "rimgui.h"
#include "profiler.h"
#include "engine.h"

#include "imgui/imgui_widget_flamegraph.h"

namespace Engine {
    namespace Render {

        static Float32 ft_array[128] = {};

        void UpdateFrametime(Float32 ft) {
            for (Sint32 i = 126; i >= 0; i--) {
                ft_array[i + 1] = ft_array[i];
            }
            ft_array[0] = ft;
        }

        static float FrametimeGetter(void* data, int idx) {
            Float32* ft_data = (Float32*)data;
            return ft_data[idx];
        }

        void DrawRendererStats() {
            RenderInfo renderer_info = {};
            GetInfo(&renderer_info);

            ImGui::Begin("Renderer stats");

            ImGui::Text("Name: %s", renderer_info.render_name);
            ImGui::Text("Renderer: %s", renderer_info.renderer);

            ImGui::Separator();

            ImGui::Text("GPU memory: %ld Kb", renderer_info.dedicated_memory >> 10);
            ImGui::Text("Shared memory: %ld Kb", renderer_info.shared_memory >> 10);

            ImGui::Separator();

            ImGui::Text("Buffers memory: %ld Kb", renderer_info.buffers_memory >> 10);
            ImGui::Text("Textures memory: %ld Kb", renderer_info.textures_memory >> 10);

            ImGui::Separator();

            ImGui::Text("Draw/Dispatch calls: %d/%d", renderer_info.r3d_draw_calls, renderer_info.r3d_dispatch_calls);

            Float32 f = 1;
            if (renderer_info.textures_inQueue != 0) {
                f = 1.0f - ((Float32)renderer_info.textures_left / (Float32)renderer_info.textures_inQueue);
            }

            ImGui::ProgressBar(f);

            ImGui::Separator();

            ImGui::Text("Fps: %.2f", 1.0f / GetDeltaTime());

            ImGui::PlotLines("Frametime", FrametimeGetter, ft_array, 128);

            ImGui::End();
        }

        static void ProfilerValueGetter(float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) {
            Profiler* prof = (Profiler*)data;

            String  section = GetProfile(idx);
            Float64 time = prof->GetTime(section);

            Float64 start = 0;
            for (Uint32 i = 0; i < idx; i++) {
                start += prof->GetTime(GetProfile(i));
            }
            if (startTimestamp) { *startTimestamp = (Float32)start * 1000; }
            if (endTimestamp) { *endTimestamp = (Float32)(start + time) * 1000; }
            if (level) { *level = 0; }
            if (caption) { *caption = section; }
        }

        void DrawProfilerStats() {
            ImGui::Begin("Main profiler");

            Profiler* prof = GetProfiler();

            ImGui::Text("Task: time ms");

            Uint32 sections = prof->GetSectionCount();
            for (Uint32 i = 0; i < sections; i++) {
                String  secsrc = GetProfile(i);
                Float64 time = prof->GetTime(secsrc) * 1000;

                ImGui::Text("%s: %.3lfms", secsrc, time);
            }

            ImGuiWidgetFlameGraph::PlotFlame("Main thread", ProfilerValueGetter, prof, sections, 0, "Main Thread", FLT_MAX, FLT_MAX, ImVec2(400, 0));

            ImGui::End();
        }
    }
}