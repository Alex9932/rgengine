#include <rgentrypoint.h>
#include <render.h>
#include <entity.h>
#include <modelsystem.h>
#include <lightsystem.h>
#include <world.h>
#include <filesystem.h>

#include <event.h>
#include <input.h>

#include <rgstring.h>

#include <camera.h>
#include <window.h>
#include <lookatcameracontroller.h>

#include <objimporter.h>
#include <pm2importer.h>
#include <pm2exporter.h>

#include <mmdimporter.h>

#include <kinematicsmodel.h>
#include <animation.h>

#include <imgui/ImGuiFileDialog.h>
#include <imgui/ImGuizmo.h>

#include <soundsystem.h>
#include <rgstb.h>

using namespace Engine;

static ObjImporter objImporter;
static PM2Importer pm2Importer;
static PMDImporter pmdimporter;

static VMDImporter vmdImporter;

static PM2Exporter pm2exporter;

static R3DStaticModelInfo modelInfo       = {};
static R3DRiggedModelInfo rmodelInfo      = {};
static KinematicsModel*   kinematicsModel = NULL;
static Animation*         animation       = NULL;

static SoundSource* src = NULL;

static StreamBuffer* stream;
static StreamBuffer* stream2;

static Bool EHandler(SDL_Event* event) {
	if (event->type != SDL_MOUSEWHEEL) { return true; }

	if (IsKeyDown(SDL_SCANCODE_LSHIFT)) {
		Float32 v = Engine::GetSoundSystem()->GetVolume();
		//v += GetMouseDW() * 0.1f;
		v += event->wheel.y * 0.1f;
		Engine::GetSoundSystem()->SetVolume(v);
		rgLogInfo(RG_LOG_SYSTEM, "Volume: %f", v);
	}

	return true;
}
static Bool EHandler2(SDL_Event* event) {
	if (event->type != SDL_KEYDOWN) { return true; }

	if (event->key.keysym.scancode == SDL_SCANCODE_R) {
		src->Pause();
	}
	if (event->key.keysym.scancode == SDL_SCANCODE_T) {
		src->Play();
	}
	if (event->key.keysym.scancode == SDL_SCANCODE_Y) {
		src->Stop();
	}

	if (event->key.keysym.scancode == SDL_SCANCODE_Q) {
		src->Stop();
		src->SetBuffer(stream);
		src->Play();
	}
	if (event->key.keysym.scancode == SDL_SCANCODE_E) {
		src->Stop();
		src->SetBuffer(stream2);
		src->Play();
	}

	return true;
}

static R3D_GlobalLightDescrition desc = {};

class Application : public BaseGame {
	private:

		Bool isModelLoaded = false;
		Bool isAnimated    = false;
		char currentModel[256];

		// Camera
		Camera*                 camera     = NULL;
		LookatCameraController* camcontrol = NULL;

		// Model entity
		Entity* ent_model = NULL;

		PointLight* l = NULL;


	public:
		Application() {
			this->isClient = true;
			this->isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_USE3D | RG_RENDER_FULLSCREEN | RG_RENDER_NOLIGHT);

			desc.ambient   = 0.4;
			desc.intensity = 6;
			desc.time      = 1.7;
			desc.color     = { 1, 0.8f, 0.7f };
		}

		~Application() {
		}

		String GetName() { return "Model editor"; }

		void MainUpdate() {

			Render::DrawRendererStats();
			Render::DrawProfilerStats();

			ImGui::Begin("Scene light");
			ImGui::SliderFloat("Time", &desc.time, 0, 6.28);
			ImGui::SliderFloat("Ambient", &desc.ambient, 0, 2);
			ImGui::SliderFloat("Intensity", &desc.intensity, 0, 20);
			ImGui::ColorPicker3("Color", desc.color.array);
			ImGui::End();

			Render::SetGlobalLight(&desc);

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

				if (isAnimated) {
					ImGui::Text("Vertices: %d", rmodelInfo.vCount);
					ImGui::Text("Indices: %d", rmodelInfo.iCount);
					ImGui::Text("Index size: %d", rmodelInfo.iType);
				} else {
					ImGui::Text("Vertices: %d", modelInfo.vCount);
					ImGui::Text("Indices: %d", modelInfo.iCount);
					ImGui::Text("Index size: %d", modelInfo.iType);
				}

				ImGuizmo::OPERATION m_op   = ImGuizmo::ROTATE;
				ImGuizmo::MODE      m_mode = ImGuizmo::LOCAL;

				vec3 pos   = {}; // UNUSED
				vec3 scale = {};
				quat rot   = {};
				mat4 view  = {};
				mat4 model = *ent_model->GetTransform()->GetMatrix();
				mat4_view(&view, camera->GetTransform()->GetPosition(), camera->GetTransform()->GetRotation());
				ImGuizmo::Manipulate(view.m, camera->GetProjection()->m, m_op, m_mode, model.m);

				//mat4_decompose(&m_pos, &m_rot, &m_scale, model);
				mat4_decompose(&pos, &rot, &scale, model);
				ent_model->GetTransform()->SetScale(scale);

				vec3 angles = rot.toEuler();
				ent_model->GetTransform()->SetRotation(angles);



				scale = ent_model->GetTransform()->GetScale();
				ImGui::InputFloat3("Scale", scale.array);
				ent_model->GetTransform()->SetScale(scale);

				ImGui::Text("Materials");
				char mat_name[64];
				Uint32 matCount = 0;
				if (isAnimated) {
					matCount = rmodelInfo.matCount;
				} else {
					matCount = modelInfo.matCount;
				}
				for (Uint32 i = 0; i < matCount; i++) {
					R3D_MaterialInfo* mat = NULL;
					if (isAnimated) {
						mat = &rmodelInfo.matInfo[i];
					} else {
						mat = &modelInfo.matInfo[i];
					}
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

					if (isAnimated) {
						RiggedModelComponent* component = ent_model->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();
						R3D_RiggedModel* mdl_handle = component->GetHandle();
						ent_model->DetachComponent(Component_RIGGEDMODELCOMPONENT);
						Render::GetModelSystem()->DeleteRiggedModelComponent(component);
						Render::R3D_DestroyRiggedModel(mdl_handle);
						pmdimporter.FreeRiggedModelData(&rmodelInfo);
						delete kinematicsModel;
					} else {
						ModelComponent* component = ent_model->GetComponent(Component_MODELCOMPONENT)->AsModelComponent();
						R3D_StaticModel* mdl_handle = component->GetHandle();
						ent_model->DetachComponent(Component_MODELCOMPONENT);
						Render::GetModelSystem()->DeleteModelComponent(component);
						Render::R3D_DestroyStaticModel(mdl_handle);
						pm2Importer.FreeModelData(&modelInfo);
					}


					isModelLoaded = false;
					isAnimated = false;
				}

				if (isAnimated && !animation) {
					ImGui::SameLine();
					if (ImGui::Button("Load animation")) {
						ImGuiFileDialog::Instance()->OpenDialog("Open animation", "Choose File", ".vmd,.anm", ".");
					}
				}
				if (isAnimated && animation) {
					if (ImGui::Button("Unload animation")) {
						kinematicsModel->GetAnimator()->PlayAnimation(NULL);
						delete animation;
						animation = NULL;
					}
				}
			} else {
				if (ImGui::Button("Load model")) {
					ImGuiFileDialog::Instance()->OpenDialog("Open model", "Choose File", ".obj,.pm2,.pmd", ".");
				}
			}
			ImGui::End();

			static vec3    color     = l->GetColor();
			static Float32 intensity = l->GetIntensity();

			ImGui::Begin("Light");
			ImGui::InputFloat("Intensity", &intensity);
			ImGui::ColorPicker3("Color", color.array);
			ImGui::End();

			l->SetColor(color);
			l->SetIntensity(intensity);

			if (ImGuiFileDialog::Instance()->Display("Open model")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
					SDL_memcpy(currentModel, filePathName.c_str(), filePathName.length() + 1);

					Engine::FS_ReplaceSeparators(currentModel);
					rgLogInfo(RG_LOG_SYSTEM, "Opened: %s", currentModel);

					ent_model->GetTransform()->SetPosition({ 0, 0, 0 });
					ent_model->GetTransform()->SetRotation({ 0, 0, 0 });
					ent_model->GetTransform()->SetScale({ 1, 1, 1 });

					if (rg_strenw(currentModel, ".obj")) {
						objImporter.ImportModel(currentModel, &modelInfo);
					} else if (rg_strenw(currentModel, ".pm2")) {
						pm2Importer.ImportModel(currentModel, &modelInfo);
					} else if (rg_strenw(currentModel, ".pmd")) {
						ent_model->GetTransform()->SetScale({ 0.1, 0.1, 0.1 });
						pmdimporter.ImportRiggedModel(currentModel, &rmodelInfo);
						kinematicsModel = pmdimporter.ImportKinematicsModel(currentModel);
						isAnimated = true;
					} else if (rg_strenw(currentModel, ".pmx")) {
						RG_ERROR_MSG("NOT IMPLEMENTED YET!");
						isAnimated = true;
					}

					if (isAnimated) {
						R3D_RiggedModel* mdl_handle = Render::R3D_CreateRiggedModel(&rmodelInfo);
						ent_model->AttachComponent(Render::GetModelSystem()->NewRiggedModelComponent(mdl_handle, kinematicsModel));
					} else {
						R3D_StaticModel* mdl_handle = Render::R3D_CreateStaticModel(&modelInfo);
						ent_model->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle));
					}


					isModelLoaded = true;

				}

				ImGuiFileDialog::Instance()->Close();
			}

			if (ImGuiFileDialog::Instance()->Display("Open animation")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

					char currentAnimation[256];
					SDL_memcpy(currentAnimation, filePathName.c_str(), filePathName.length() + 1);

					Engine::FS_ReplaceSeparators(currentAnimation);
					rgLogInfo(RG_LOG_SYSTEM, "Opened: %s", currentAnimation);

					if (rg_strenw(currentAnimation, ".vmd")) {
						animation = vmdImporter.ImportAnimation(currentAnimation, kinematicsModel);
						animation->SetRepeat(true);
						kinematicsModel->GetAnimator()->PlayAnimation(animation);
					}
					else if (rg_strenw(currentAnimation, ".anm")) {
						RG_ERROR_MSG("NOT IMPLEMENTED YET!");
					}
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

			if (isAnimated && kinematicsModel) {
				kinematicsModel->GetAnimator()->Update(GetDeltaTime());
				kinematicsModel->RebuildSkeleton();
				kinematicsModel->SolveCCDIK();
				kinematicsModel->RecalculateTransform();
			}


			// Draw animated model
			RiggedModelComponent* rcomponent = ent_model->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();
			if (rcomponent) {

				R3DUpdateBufferInfo binfo = {};
				binfo.offset = 0;
				binfo.data   = kinematicsModel->GetTransforms();
				binfo.handle = kinematicsModel->GetBufferHandle();
				binfo.length = sizeof(mat4) * kinematicsModel->GetBoneCount();
				Render::R3D_UpdateBoneBuffer(&binfo);
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
			//objImporter.ImportModel("gamedata/greenscreen/scene.obj", &objinfo);
			//R3D_StaticModel* mdl_handle0 = Render::R3D_CreateStaticModel(&objinfo);
			//objImporter.FreeModelData(&objinfo);

			pm2Importer.ImportModel("gamedata/greenscreen/scene.pm2", &objinfo);
			R3D_StaticModel* mdl_handle0 = Render::R3D_CreateStaticModel(&objinfo);
			pm2Importer.FreeModelData(&objinfo);

			Entity* ent_bg = world->NewEntity();
			ent_bg->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));
			ent_bg->GetTransform()->SetPosition({ 0, 0, 0 });
			ent_bg->GetTransform()->SetRotation({ 0, 0, 0 });
			ent_bg->GetTransform()->SetScale({ 1, 1, 1 });

			l = Render::GetLightSystem()->NewPointLight();
			l->SetColor({ 1, 0.9, 0.8 });
			l->SetIntensity(50);
			l->SetOffset({ -1.86, 2.96, 1.81 });
			ent_bg->AttachComponent(l);


			ent_model = world->NewEntity();
			ent_model->GetTransform()->SetPosition({ 0, 0, 0 });
			ent_model->GetTransform()->SetRotation({ 0, 0, 0 });
			ent_model->GetTransform()->SetScale({ 1, 1, 1 });

			SoundSystem* ss = Engine::GetSoundSystem();
			
			src = ss->NewSoundSource();
			ent_bg->AttachComponent(src);

			stream  = new StreamBuffer("gamedata/sounds/music/caramellooped.ogg");
			stream2 = new StreamBuffer("gamedata/sounds/music/chipi chipi chapa chapa(looped).ogg");

			src->SetBuffer(stream);
			src->SetRepeat(true);
			//src->Play();

			RegisterEventHandler(EHandler);
			RegisterEventHandler(EHandler2);

		}

		void Quit() {

			RG_DELETE_CLASS(GetDefaultAllocator(), LookatCameraController, camcontrol);
			RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);

			src->Stop();
			delete stream;
			delete stream2;

			World* world = GetWorld();

			///////////////////////////////////////////////////////////////////

			if (animation) {
				delete animation;
			}


			world->ClearWorld();
		}
};

int EntryPoint(int argc, String* argv) {

	Application app;
	Initialize(&app);
	Start();

	return 0;

}

rgmain(EntryPoint)