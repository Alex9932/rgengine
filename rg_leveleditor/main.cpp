#include <rgentrypoint.h>

#include <allocator.h>
#include <camera.h>

#include <render.h>
#include <modelsystem.h>
#include <lightsystem.h>
#include <world.h>
#include <imgui/imgui.h>

#include "viewport.h"

// Temp
#include <objimporter.h>


using namespace Engine;

class Application : public BaseGame {
	private:
		Viewport* viewport = NULL;
		// Other components

		Camera*   camera   = NULL;

		// Temp
		Entity*   ent0     = NULL;

	public:
		Application() {
			this->isClient   = true;
			this->isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_USE3D | RG_RENDER_FULLSCREEN);
		}

		~Application() {
		}

		void MainUpdate() {

			// Update camera
			camera->Update(GetDeltaTime());
			R3D_CameraInfo cam = {};
			cam.projection = *camera->GetProjection();
			cam.position = camera->GetTransform()->GetPosition();
			cam.rotation = camera->GetTransform()->GetRotation();
			Render::R3D_SetCamera(&cam);

			///////////
			R3D_PushModelInfo info = {};
			info.handle = ent0->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle();
			info.matrix = *ent0->GetTransform()->GetMatrix();
			Render::R3D_PushModel(&info); 

			////////

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

		
			viewport->DrawComponent();


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

				progress += GetDeltaTime() * 0.333f;

				if (progress > 1) {
					testDisable = false;
				}

			}


			if (isStats) {
				RenderInfo renderer_info = {};
				Render::GetInfo(&renderer_info);

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

				ImGui::Text("Fps: %.2f", 1.0f / GetDeltaTime());

				ImGui::End();

			}

		}

		void Initialize() {

			World* world = GetWorld();

			// Initialize camera
			camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 100, rgToRadians(75), 1.777f);
			camera->GetTransform()->SetPosition({ 0, 0.85f, 0 });
			camera->GetTransform()->SetRotation({ 0, 0, 0 });

			// Viewport
			viewport = new Viewport(camera);

			// Temp
			Render::ObjImporter objImporter;
			R3DCreateStaticModelInfo objinfo = {};
			objImporter.ImportModel("gamedata/models/megumin/megumin_v4.obj", &objinfo);
			R3D_StaticModel* mdl_handle0 = Render::R3D_CreateStaticModel(&objinfo);
			objImporter.FreeModelData(&objinfo);

			ent0 = world->NewEntity();
			ent0->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));
			ent0->GetTransform()->SetPosition({ 0, 0, -1.6 });
			ent0->GetTransform()->SetRotation({ 0, 0, 0 });
			ent0->GetTransform()->SetScale({ 1, 1, 1 });

			PointLight* l = Render::GetLightSystem()->NewPointLight();
			l->SetColor({ 1, 0.9, 0.8 });
			l->SetIntensity(10);
			l->SetOffset({ -0.86, 2.56, 1.21 });
			ent0->AttachComponent(l);

		}

		void Quit() {
			delete viewport;

			RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);

			World* world = GetWorld();
			Render::R3D_DestroyStaticModel(ent0->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle());
			world->FreeEntity(ent0);
		}
};

int EntryPoint(int argc, String* argv) {

	Application app;
	Initialize(&app);
	Start();

	return 0;

}

rgmain(EntryPoint)