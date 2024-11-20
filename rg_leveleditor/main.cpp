#define GAME_DLL
#include <rgentrypoint.h>

#include <event.h>
#include <input.h>

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

static ImGuizmo::OPERATION guizmo_modes[] = {
	ImGuizmo::TRANSLATE,
	ImGuizmo::ROTATE,
	ImGuizmo::TRANSLATE | ImGuizmo::ROTATE
};

static Uint32 guizmo_mode = 0;

static Bool EHandler(SDL_Event* event) {

	if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_TAB) {
		guizmo_mode++;
		guizmo_mode = guizmo_mode % (sizeof(guizmo_modes) / sizeof(ImGuizmo::OPERATION));
	}

	return true;
}

class Application : public BaseGame {
	private:
		Viewport*   viewport   = NULL;
		EntityList* entitylist = NULL;
		// Other components

		Camera*               camera     = NULL;
		FreeCameraController* camcontrol = NULL;

		World* world = NULL;

	public:
		Application() {
			this->isClient   = true;
			this->isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_USE3D /*| RG_RENDER_NOLIGHT*/ | RG_RENDER_NOPOSTPROCESS);
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
			Render::SetCamera(&cam);


			ImGuizmo::BeginFrame();
			viewport->SetImGuizmoRect();

			DockerBegin();
			MenubarDraw();
			if (PopupShown()) { ImGui::BeginDisabled(); }
			

			////////////////////////////
			// In docker windows      //
			////////////////////////////


			// TODO: External window class
			ImGui::Begin("Window 1");
			ImGui::Text("Some useful feature");
			ImGui::Text("Useful information");
			ImGui::Button("Some useful button");
			ImGui::SameLine();
			ImGui::Button("Other useful button");
			ImGui::End();

			entitylist->DrawComponent();

			UUID entId = entitylist->GetActiveEntity();
			if (entId != 0) {
				Entity* ent = world->GetEntityByUUID(entId);
				viewport->Manipulate(ent->GetTransform()->GetMatrix(), guizmo_modes[guizmo_mode], ImGuizmo::WORLD);
			}

			// TODO: External window class
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

			// ManipulateResult is available AFTER component redraw
			if (viewport->IsManipulationResult() && entId != 0) {
				ManipulateResult result;
				viewport->GetManipulateResult(&result);

				Entity* ent = world->GetEntityByUUID(entId);
				Transform* transform = ent->GetTransform();

				//vec3 r = result.rot.toEuler();
				//transform->SetRotation(r);
				//transform->SetPosition(result.pos);
				//transform->Recalculate();

				transform->SetMatrix(&result.matrix);
			}

			
			if (PopupShown()) { ImGui::EndDisabled(); }
			DockerEnd();
			PopupDraw();

			if (docker_isStats) { Render::DrawRendererStats(); }

		}

		void Initialize() {

			SetFpsLimit(60);
			RegisterEventHandler(EHandler);
			
			world = GetWorld();

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