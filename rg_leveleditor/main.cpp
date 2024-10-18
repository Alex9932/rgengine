#define GAME_DLL
#include <rgentrypoint.h>

#include <allocator.h>
#include <camera.h>
#include <freecameracontroller.h>

#include <render.h>
#include <window.h>
#include <modelsystem.h>
#include <lightsystem.h>
#include <world.h>
#include <imgui/imgui.h>

// Windows
#include "viewport.h"
#include "entitylist.h"

#include "levelexporter.h"


#include "popup.h"

using namespace Engine;

static R3D_GlobalLightDescrition globaLightDesc = {
	{1, 0.9f, 0.8f},
	0.747f,
	7,
	0.3f,
	1.86f
};

class Application : public BaseGame {
	private:
		Viewport*   viewport   = NULL;
		EntityList* entitylist = NULL;
		// Other components

		Camera*               camera     = NULL;
		FreeCameraController* camcontrol = NULL;

	public:
		Application() {
			this->isClient   = true;
			this->isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_USE3D | RG_RENDER_NOLIGHT | RG_RENDER_NOPOSTPROCESS);
		}

		~Application() {
		}

		String GetName() { return "Level editor"; }

		void MainUpdate() {

			// Update camera
			camcontrol->Update();
			camera->Update(GetDeltaTime());
			R3D_CameraInfo cam = {};
			cam.projection = *camera->GetProjection();
			cam.position = camera->GetTransform()->GetPosition();
			cam.rotation = camera->GetTransform()->GetRotation();
			Render::R3D_SetCamera(&cam);


			ImGuizmo::BeginFrame();
			viewport->SetImGuizmoRect();

			static Bool isStats = false;

			////////////////////////////
			// Docker                 //
			////////////////////////////
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
				window_flags |=  ImGuiWindowFlags_NoNavFocus;
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

			////////////////////////////
			// Menubar                //
			////////////////////////////

			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("Exit")) { Engine::Quit(); }
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Render")) {
					if (ImGui::MenuItem("Nope")) {}
					if (ImGui::MenuItem("Toggle renderer stats", "", isStats)) { isStats = !isStats; }
					//if (ImGui::MenuItem("Test disable")) {
					//	popupWindow = true;
					//}
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

			////////////////////////////
			// In docker windows      //
			////////////////////////////


			if (PopupShown()) {
				ImGui::BeginDisabled();
			}


			ImGui::Begin("Window 1");
			ImGui::Text("Text 1");
			ImGui::Text("Text 2");
			ImGui::Text("Text 3");
			ImGui::End();

			entitylist->DrawComponent();


			ImGui::Begin("Global light");
			//static vec4 color;

			ImGui::SliderFloat("Time", &globaLightDesc.time, 0, 6.283);
			ImGui::SliderFloat("Ambient", &globaLightDesc.ambient, 0, 1);
			ImGui::SliderFloat("Intensity", &globaLightDesc.intensity, 0, 20);
			ImGui::SliderFloat("Turbidity", &globaLightDesc.turbidity, 0, 6);
			ImGui::ColorPicker4("Color", globaLightDesc.color.array);

			Render::SetGlobalLight(&globaLightDesc);

			ImGui::End();

			ImVec2 padding = { 0, 0 };
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
			viewport->DrawComponent();
			ImGui::PopStyleVar();

			
			if (PopupShown()) {
				ImGui::EndDisabled();
			}

			////////////////////////////
			// Docker end             //
			////////////////////////////
			ImGui::End();

			PopupDraw();

			if (isStats) {
				Render::DrawRendererStats();
			}

		}

		void Initialize() {

			SetFpsLimit(100);
			

			World* world = GetWorld();

			// Initialize camera
			camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 1000, rgToRadians(75), 1.777f);
			camera->GetTransform()->SetPosition({ 0, 0.85f, 1.6 });
			camera->GetTransform()->SetRotation({ 0, 0, 0 });

			camcontrol = RG_NEW_CLASS(GetDefaultAllocator(), FreeCameraController)(camera);

			// Windows
			viewport = new Viewport(camera);
			entitylist = new EntityList();

			// Temp
			R3DStaticModelInfo objinfo = {};
			//objImporter.ImportModel("gamedata/greenscreen/scene.obj", &objinfo);
			//objImporter.ImportModel("gamedata/background/scene.obj", &objinfo);

			/*
			pm2Importer.ImportModel("gamedata/models/skybox/geometry.pm2", &objinfo);
			R3D_StaticModel* mdl_handle0 = Render::R3D_CreateStaticModel(&objinfo);
			pm2Importer.FreeModelData(&objinfo);
			*/

			/*
			Entity* ent0 = world->NewEntity();
			ent0->GetComponent(Component_TAG)->AsTagComponent()->SetString("Skybox");
			ent0->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));
			ent0->GetTransform()->SetPosition({ 1, 0, 8 });
			ent0->GetTransform()->SetRotation({ 0, 0, 0 });
			ent0->GetTransform()->SetScale({ 1, 1, 1 });
			*/

#if 0
			pm2Importer.ImportModel("gamedata/flatplane/scene.pm2", &objinfo);
			R3D_StaticModel* mdl_handle1 = Render::R3D_CreateStaticModel(&objinfo);
			pm2Importer.FreeModelData(&objinfo);

			Entity* ent1 = world->NewEntity();
			ent1->GetComponent(Component_TAG)->AsTagComponent()->SetString("Ground");
			ent1->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle1));
			ent1->SetAABB(&objinfo.aabb);
			ent1->GetTransform()->SetPosition({ 0, 0, 1 });
			ent1->GetTransform()->SetRotation({ 0, 0, 0 });
			ent1->GetTransform()->SetScale({ 1, 1, 1 });
#endif

#if 0
			objImporter.ImportModel("gamedata/models/mosaic/mosaic.obj", &objinfo);
			R3D_StaticModel* mdl_handle2 = Render::R3D_CreateStaticModel(&objinfo);
			objImporter.FreeModelData(&objinfo);

			Entity* ent2 = world->NewEntity();
			ent2->GetComponent(Component_TAG)->AsTagComponent()->SetString("Model");
			ent2->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle2));
			ent2->SetAABB(&objinfo.aabb);
			ent2->GetTransform()->SetPosition({ 0, 2.5f, 0 });
			ent2->GetTransform()->SetRotation({ 0, (3.1415f / 2) + 3.1415f, 0 });
			ent2->GetTransform()->SetScale({ 1, 1, 1 });
#endif
			/*
			PointLight* l = Render::GetLightSystem()->NewPointLight();
			l->SetColor({ 1, 0.9, 0.8 });
			l->SetIntensity(30);
			l->SetOffset({ -0.86, 2.56, 0.21 });
			ent0->AttachComponent(l);
			*/

		}

		void Quit() {
			delete viewport;
			delete entitylist;

			RG_DELETE_CLASS(GetDefaultAllocator(), FreeCameraController, camcontrol);
			RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);

			World* world = GetWorld();

			world->ClearWorld();

		}
};

static Application* app;

void Module_Initialize() {
	app = new Application();
}

void Module_Destroy() {
	delete app;
}

BaseGame* Module_GetApplication() {
	return app;
}