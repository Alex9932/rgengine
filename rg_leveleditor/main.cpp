#define GAME_DLL
#include <rgentrypoint.h>

#include <allocator.h>
#include <camera.h>
#include <freecameracontroller.h>

#include <render.h>
#include <window.h>
#include <world.h>
#include <imgui/imgui.h>

// Windows
#include "viewport.h"
#include "entitylist.h"
#include "docker.h"
#include "menubar.h"
#include "popup.h"

#include "dockerglobal.h"

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

			DockerBegin();
			MenubarDraw();
			if (PopupShown()) { ImGui::BeginDisabled(); }
			

			////////////////////////////
			// In docker windows      //
			////////////////////////////



			ImGui::Begin("Window 1");
			ImGui::Text("Text 1");
			ImGui::Text("Text 2");
			ImGui::Text("Text 3");
			ImGui::End();

			entitylist->DrawComponent();


			ImGui::Begin("Global light");

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

			
			if (PopupShown()) { ImGui::EndDisabled(); }
			DockerEnd();
			PopupDraw();

			if (docker_isStats) { Render::DrawRendererStats(); }

		}

		void Initialize() {

			SetFpsLimit(60);
			
			World* world = GetWorld();

			// Initialize camera
			camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 1000, rgToRadians(75), 1.777f);
			camera->GetTransform()->SetPosition({ 0, 0.85f, 1.6 });
			camera->GetTransform()->SetRotation({ 0, 0, 0 });

			camcontrol = RG_NEW_CLASS(GetDefaultAllocator(), FreeCameraController)(camera);

			// Windows
			viewport = new Viewport(camera);
			entitylist = new EntityList();

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