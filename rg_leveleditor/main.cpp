#include <rgentrypoint.h>

#include <render.h>
#include <imgui/imgui.h>

class Application : public Engine::BaseGame {
	public:
		Application() {
			this->isClient = true;
			this->isGraphics = true;
		}

		~Application() {
		}

		void MainUpdate() {

			static Float32 progress = 0;
			static Bool testDisable = false;

			static Bool isStats = false;

			// Docker
			static bool opt_fullscreen = true;
			static bool opt_padding = false;
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

			if (opt_fullscreen) {
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->WorkPos);
				ImGui::SetNextWindowSize(viewport->WorkSize);
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			} else {
				dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
			}

			if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
				window_flags |= ImGuiWindowFlags_NoBackground;
			}

			if (!opt_padding) {
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			}

			ImGui::Begin("DockSpace Demo", NULL, window_flags);

			if (!opt_padding) {
				ImGui::PopStyleVar();
			}

			if (opt_fullscreen) {
				ImGui::PopStyleVar(2);
			}

			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
				ImGuiID dockspace_id = ImGui::GetID("DockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}

			///////////////

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("Exit")) { Engine::Quit(); }
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Render")) {
					if (ImGui::MenuItem("Nope")) {}
					if (ImGui::MenuItem("Toggle renderer stats", "", isStats)) { isStats = !isStats; }
					if (ImGui::MenuItem("Test disable")) {
						progress    = 0;
						testDisable = true;
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Docker")) {
					// Disabling fullscreen would allow the window to be moved to the front of other windows,
					// which we can't undo at the moment without finer window depth/z control.
					ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
					ImGui::MenuItem("Padding", NULL, &opt_padding);
					ImGui::Separator();

					if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
					if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
					if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
					if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0)) { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }
					if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen)) { dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode; }
					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			///////////////

			if (testDisable) {
				ImGui::BeginDisabled();
			}


			ImGui::Begin("Window 1");
			ImGui::Text("Text 1");
			ImGui::Text("Text 2");
			ImGui::Text("Text 3");
			ImGui::End();

			ImGui::Begin("Window 2");
			ImGui::Text("Text 1");
			ImGui::Text("Text 2");
			ImGui::Text("Text 3");
			ImGui::End();

			ImGui::Begin("Window 3");
			static vec4 color;
			ImGui::ColorPicker4("Select color", color.array);
			ImGui::End();


			if (testDisable) {
				ImGui::EndDisabled();
			}

			// Docker end

			ImGui::End();


			if (testDisable) {

				//ImGui::PushStyleVar(ImGuiStyleVar_WindowRe, );
				ImGui::SetNextWindowPos({ 670, 410 });
				ImGui::SetNextWindowSize({ 260, 80 });
				ImGui::Begin("Process");

				ImGui::Text("Working");
				ImGui::ProgressBar(progress);

				ImGui::End();

				progress += Engine::GetDeltaTime() * 0.333f;

				if (progress > 1) {
					testDisable = false;
				}

			}


			if (isStats) {
				RenderInfo renderer_info = {};
				Engine::Render::GetInfo(&renderer_info);

				ImGui::Begin("Renderer stats");

				ImGui::Text("Name: %s", renderer_info.render_name);
				ImGui::Text("Renderer: %s", renderer_info.renderer);

				ImGui::Separator();

				ImGui::Text("Buffers memory: %ld Kb", renderer_info.buffers_memory >> 10);
				ImGui::Text("Models loaded: %d", renderer_info.meshes_loaded);

				ImGui::Separator();

				ImGui::Text("Draw/Dispatch calls: %d/%d", renderer_info.r3d_draw_calls, renderer_info.r3d_dispatch_calls);

				ImGui::Separator();

				ImGui::Text("Textures memory: %ld Kb", renderer_info.textures_memory >> 10);
				ImGui::Text("Textures loaded: %d", renderer_info.textures_loaded);
				ImGui::Text("Textures to load/queued: %d/%d", renderer_info.textures_inQueue, renderer_info.textures_left);

				Float32 f = 1;
				if (renderer_info.textures_inQueue != 0) {
					f = 1.0f - ((Float32)renderer_info.textures_left / (Float32)renderer_info.textures_inQueue);
				}

				ImGui::ProgressBar(f);

				ImGui::Separator();

				ImGui::Text("Fps: %.2f", 1.0f / Engine::GetDeltaTime());

				ImGui::End();

			}

		}

		void Initialize() {
		}

		void Quit() {
		}
};

int EntryPoint(int argc, String* argv) {

	Application app;
	Engine::Initialize(&app);
	Engine::Start();

	return 0;

}

rgmain(EntryPoint)