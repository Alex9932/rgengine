#include <rgentrypoint.h>
#include <render.h>
#include <entity.h>
#include <modelsystem.h>
#include <lightsystem.h>
#include <world.h>

#include <rgstring.h>

#include <camera.h>
#include <window.h>
#include <lookatcameracontroller.h>

#include <objimporter.h>
#include <pm2importer.h>
#include <pm2exporter.h>

#include <imgui/ImGuiFileDialog.h>
#include <imgui/ImGuizmo.h>

using namespace Engine;

static ObjImporter objImporter;
static PM2Importer pm2Importer;

static PM2Exporter pm2exporter;

static R3DStaticModelInfo modelInfo = {};

class Application : public BaseGame {
	private:

		Bool isModelLoaded = false;
		char currentModel[256];

		// Camera
		Camera*                 camera     = NULL;
		LookatCameraController* camcontrol = NULL;

		// Background entity
		Entity* ent_bg    = NULL;

		// Model entity
		Entity* ent_model = NULL;


	public:
		Application() {
			this->isClient = true;
			this->isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_USE3D | RG_RENDER_FULLSCREEN);
		}

		~Application() {
		}

		String GetName() { return "Model editor"; }

		void MainUpdate() {

			// Update camera
			camcontrol->Update();
			camera->Update(GetDeltaTime());
			R3D_CameraInfo cam = {};
			cam.projection = *camera->GetProjection();
			cam.position = camera->GetTransform()->GetPosition();
			cam.rotation = camera->GetTransform()->GetRotation();
			Render::R3D_SetCamera(&cam);

			ivec2 scrsize;
			GetWindowSize(&scrsize);

			ImGuizmo::BeginFrame();
			ImGuizmo::SetRect(0, 0, scrsize.x, scrsize.y);

			ImGui::Begin("Model");

			if (isModelLoaded) {
				ImGui::Text("Current model: %s", currentModel);
				// TODO

				ImGui::Text("Vertices: %d", modelInfo.vCount);
				ImGui::Text("Indices: %d", modelInfo.iCount);
				ImGui::Text("Index size: %d", modelInfo.iType);

				ImGuizmo::OPERATION m_op   = ImGuizmo::SCALE;
				ImGuizmo::MODE      m_mode = ImGuizmo::LOCAL;

				vec3 scale = {};
				mat4 view  = {};
				mat4 model = *ent_model->GetTransform()->GetMatrix();
				mat4_view(&view, camera->GetTransform()->GetPosition(), camera->GetTransform()->GetRotation());
				ImGuizmo::Manipulate(view.m, camera->GetProjection()->m, m_op, m_mode, model.m);

				//mat4_decompose(&m_pos, &m_rot, &m_scale, model);
				mat4_decompose(NULL, NULL, &scale, model);
				ent_model->GetTransform()->SetScale(scale);

				scale = ent_model->GetTransform()->GetScale();
				ImGui::InputFloat3("Scale", scale.array);
				ent_model->GetTransform()->SetScale(scale);

				ImGui::Text("Materials");
				char mat_name[64];
				for (Uint32 i = 0; i < modelInfo.matCount; i++) {
					R3D_MaterialInfo* mat = &modelInfo.matInfo[i];
					SDL_snprintf(mat_name, 64, "Material %d", i);
					if (ImGui::TreeNode(mat_name)) {
						ImGui::InputText("Albedo", mat->albedo, 128);
						ImGui::InputText("Normal", mat->normal, 128);
						ImGui::InputText("PBR", mat->pbr, 128);
						ImGui::ColorEdit3("Color", mat->color.array);
						ImGui::TreePop();
					}
				}



			}

			if (isModelLoaded) {
				if (ImGui::Button("Save as")) {
					ImGuiFileDialog::Instance()->OpenDialog("Save model", "Choose File", ".pm2", ".");
				}
				ImGui::SameLine();
				if (ImGui::Button("Close")) {

					ModelComponent* component   = ent_model->GetComponent(Component_MODELCOMPONENT)->AsModelComponent();
					R3D_StaticModel* mdl_handle = component->GetHandle();

					ent_model->DetachComponent(Component_MODELCOMPONENT);
					Render::GetModelSystem()->DeleteModelComponent(component);

					Render::R3D_DestroyStaticModel(mdl_handle);

					pm2Importer.FreeModelData(&modelInfo);
					isModelLoaded = false;
				}
			} else {
				if (ImGui::Button("Load model")) {
					ImGuiFileDialog::Instance()->OpenDialog("Open model", "Choose File", ".obj,.pm2", ".");
				}
			}
			ImGui::End();

			static vec3    color     = {};
			static Float32 intensity = 0;

			ImGui::Begin("Light");
			ImGui::InputFloat("Intensity", &intensity);
			ImGui::ColorPicker3("Color", color.array);
			ImGui::End();

			if (ImGuiFileDialog::Instance()->Display("Open model")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

					SDL_memcpy(currentModel, filePathName.c_str(), filePathName.length() + 1);
					rgLogInfo(RG_LOG_SYSTEM, "Opened: %s", currentModel);

					if (rg_strenw(currentModel, ".obj")) {
						objImporter.ImportModel(currentModel, &modelInfo);
					} else if (rg_strenw(currentModel, ".pm2")) {
						pm2Importer.ImportModel(currentModel, &modelInfo);
					}

					R3D_StaticModel* mdl_handle = Render::R3D_CreateStaticModel(&modelInfo);
					ent_model->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle));

					ent_model->GetTransform()->SetPosition({ 0, 0, 0 });
					ent_model->GetTransform()->SetRotation({ 0, 0, 0 });
					ent_model->GetTransform()->SetScale({ 1, 1, 1 });

					isModelLoaded = true;

				}

				ImGuiFileDialog::Instance()->Close();
			}


			if (ImGuiFileDialog::Instance()->Display("Save model")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
					pm2exporter.ExportModel(filePathName.c_str(), &modelInfo, ent_model->GetTransform()->GetMatrix());
				}
				ImGuiFileDialog::Instance()->Close();
			}



			// TEMP
			R3D_PushModelInfo info = {};
			info.handle = ent_bg->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle();
			info.matrix = *ent_bg->GetTransform()->GetMatrix();
			Render::R3D_PushModel(&info);

			ModelComponent* component = ent_model->GetComponent(Component_MODELCOMPONENT)->AsModelComponent();
			if (component) {
				info.handle = component->GetHandle();
				info.matrix = *ent_model->GetTransform()->GetMatrix();
				Render::R3D_PushModel(&info);
			}

		}

		void Initialize() {

			World* world = GetWorld();

			// Initialize camera
			camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 1000, rgToRadians(75), 1.777f);
			camera->GetTransform()->SetPosition({ 0, 0.85f, 1.6 });
			camera->GetTransform()->SetRotation({ 0, 0, 0 });

			camcontrol = RG_NEW_CLASS(GetDefaultAllocator(), LookatCameraController)(camera);


			R3DStaticModelInfo objinfo = {};
			objImporter.ImportModel("gamedata/greenscreen/scene.obj", &objinfo);
			R3D_StaticModel* mdl_handle0 = Render::R3D_CreateStaticModel(&objinfo);
			objImporter.FreeModelData(&objinfo);

			ent_bg = world->NewEntity();
			ent_bg->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));
			ent_bg->GetTransform()->SetPosition({ 0, 0, 0 });
			ent_bg->GetTransform()->SetRotation({ 0, 0, 0 });
			ent_bg->GetTransform()->SetScale({ 1, 1, 1 });

			PointLight* l = Render::GetLightSystem()->NewPointLight();
			l->SetColor({ 1, 0.9, 0.8 });
			l->SetIntensity(50);
			l->SetOffset({ -1.86, 2.96, 1.81 });
			ent_bg->AttachComponent(l);


			ent_model = world->NewEntity();
			ent_model->GetTransform()->SetPosition({ 0, 0, 0 });
			ent_model->GetTransform()->SetRotation({ 0, 0, 0 });
			ent_model->GetTransform()->SetScale({ 1, 1, 1 });

		}

		void Quit() {

			RG_DELETE_CLASS(GetDefaultAllocator(), LookatCameraController, camcontrol);
			RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);

			World* world = GetWorld();
			Render::GetLightSystem()->DeletePointLight(ent_bg->GetComponent(Component_POINTLIGHT)->AsPointLightComponent());
			Render::R3D_DestroyStaticModel(ent_bg->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle());
			world->FreeEntity(ent_bg);

			world->FreeEntity(ent_model);
		}
};

int EntryPoint(int argc, String* argv) {

	Application app;
	Initialize(&app);
	Start();

	return 0;

}

rgmain(EntryPoint)