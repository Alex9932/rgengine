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
#include <imgui/ImGuizmo.h>

// Windows
#include "viewport.h"
#include "entitylist.h"
#include "staticlist.h"
#include "docker.h"
#include "menubar.h"
#include "popup.h"

#include "dockerglobal.h"

#include "staticobject.h"

using namespace Engine;

static R3D_GlobalLightDescrition globaLightDesc = {
	{1, 0.9f, 0.8f},
	0.747f,
	7,
	0.3f,
	1.86f
};

static Uint32 gizmo_op   = 0;
static Uint32 gizmo_mode = 0;

static Viewport* viewport;

static Bool EHandler(SDL_Event* event) {

	if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_TAB) {
		if (IsKeyDown(SDL_SCANCODE_LSHIFT)) {
			gizmo_mode++;
			gizmo_mode = gizmo_mode % 2;
			viewport->SetGizmoMode(gizmo_mode);
		} else {
			gizmo_op++;
			gizmo_op = gizmo_op % 3;
			viewport->SetGizmoOp(gizmo_op);
		}
	}

	return true;
}

class Application : public BaseGame {
	private:
		EntityList* entitylist = NULL;
		StaticList* staticlist = NULL;
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

			staticlist->DrawComponent();
			entitylist->DrawComponent();

			UUID gizmoId = viewport->GetGizmoID();
			if (gizmoId != 0) {
				Entity* ent = world->GetEntityByUUID(gizmoId);
				StaticObject* obj = world->GetStaticObjectByUUID(gizmoId);
				mat4* mat = NULL;

				if (ent) { mat = ent->GetTransform()->GetMatrix(); }
				if (obj) { mat = obj->GetMatrix(); }

				// Manipulate
				if (mat) {
					viewport->Manipulate(mat);
				}
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
			if (viewport->IsManipulationResult() && gizmoId != 0) {
				ManipulateResult result;
				viewport->GetManipulateResult(&result);

				Entity* ent = world->GetEntityByUUID(gizmoId);
				StaticObject* obj = world->GetStaticObjectByUUID(gizmoId);

				if (ent) {
					Transform* transform = ent->GetTransform();

					//vec3 r = result.rot.toEuler();
					//transform->SetRotation(r);
					//transform->SetPosition(result.pos);
					//transform->Recalculate();

					transform->SetMatrix(&result.matrix);
				}

				if (obj) {
					// Just copy matrix
					SDL_memcpy(obj->GetMatrix(), &result.matrix, sizeof(mat4));
				}
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
			entitylist = new EntityList(viewport);
			staticlist = new StaticList(viewport);

		}

		void Quit() {
			delete entitylist;
			delete staticlist;
			delete viewport;

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