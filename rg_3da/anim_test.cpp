#if 1

#include <engine.h>
#include <rgentrypoint.h>

#include <render.h>
#include <window.h>
#include <modelsystem.h>

#include <world.h>
#include <camera.h>
#include <kinematicsmodel.h>
#include <lookatcameracontroller.h>

// MMD tools
#include <mmdimporter.h>

using namespace Engine;


static Camera*                 camera         = NULL;
static LookatCameraController* cam_controller = NULL;

static Entity*                 player         = NULL;
static KinematicsModel*        kmodel         = NULL;

static R3D_GlobalLightDescrition desc = {};


// Animations
static Animation* anim[9] = {};

static int val = 0;
static const char* items[] = { "None", "Stand", "Walk", "Run", "Squat", "Sneaking", "Idle", "Do Idle 1" , "Do Idle 2" , "Do Idle 3" };

class Application : public BaseGame {
	public:
		Application()  {
			isClient = true;
			isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_FULLSCREEN | RG_RENDER_USE3D);

			desc.color = { 1, 1, 1 };
			desc.ambient = 0.45f;
			desc.intensity = 3.3f;
			desc.turbidity = 1.86f;
			desc.time = 2.33f;
		}

		~Application() {}

		void MainUpdate() {

			ImGui::Begin("Scene light");
			ImGui::SliderFloat("Time", &desc.time, 0, 6.28);
			ImGui::SliderFloat("Ambient", &desc.ambient, 0, 2);
			ImGui::SliderFloat("Intensity", &desc.intensity, 0, 20);
			ImGui::SliderFloat("Turbidity", &desc.turbidity, 0, 5);
			ImGui::ColorPicker3("Color", desc.color.array);
			ImGui::End();


			ImGui::Begin("Animation control");

			if (ImGui::Combo("Animation", &val, items, IM_ARRAYSIZE(items))) {
				if (val == 0) {
					kmodel->GetAnimator()->PlayAnimation(NULL);
				} else {
					kmodel->GetAnimator()->PlayAnimation(anim[val - 1]);
				}
			}

			ImGui::End();

			Render::SetGlobalLight(&desc);
		
			// Recalculate projection
			ivec2 size = {};
			Engine::GetWindowSize(&size);
			camera->SetAspect((Float32)size.x / (Float32)size.y);
			camera->ReaclculateProjection();

			// Update camera
			cam_controller->Update();
			camera->Update(GetDeltaTime());

			R3D_CameraInfo cam = {};
			cam.projection = *camera->GetProjection();
			cam.position   = camera->GetTransform()->GetPosition();
			cam.rotation   = camera->GetTransform()->GetRotation();
			Render::SetCamera(&cam);
		
			// Calculate skeleton
			kmodel->GetAnimator()->Update(GetDeltaTime());
			kmodel->RebuildSkeleton();
			kmodel->SolveCCDIK();
			kmodel->RecalculateTransform();

			R3DUpdateBufferInfo binfo = {};
			binfo.offset = 0;
			binfo.data   = kmodel->GetTransforms();
			binfo.handle = kmodel->GetBufferHandle();
			binfo.length = sizeof(mat4) * kmodel->GetBoneCount();
			Render::R3D_UpdateBoneBuffer(&binfo);

		}

		void Initialize() {
		
			World* world = GetWorld();

			// Create 3-rd person camera
			camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 1000, rgToRadians(75), 1.777f);
			cam_controller = RG_NEW_CLASS(GetDefaultAllocator(), LookatCameraController)(camera);


			PMXImporter pmxImporter;
			PMDImporter pmdImporter;
			VMDImporter vmdImporter;

			// Load geometry
			R3DRiggedModelInfo info = {};
			pmdImporter.ImportRiggedModel("mmd_models/Rin_Kagamine.pmd", &info);
			R3D_RiggedModel* mdl_handle = Render::R3D_CreateRiggedModel(&info);
			pmdImporter.FreeRiggedModelData(&info);
			kmodel = pmdImporter.ImportKinematicsModel("mmd_models/Rin_Kagamine.pmd");

			// Load animations
			anim[0] = vmdImporter.ImportAnimation("vmd/player/stand.vmd", kmodel);
			anim[1] = vmdImporter.ImportAnimation("vmd/player/walk.vmd", kmodel);
			anim[2] = vmdImporter.ImportAnimation("vmd/player/run.vmd", kmodel);
			anim[3] = vmdImporter.ImportAnimation("vmd/player/squat.vmd", kmodel);
			anim[4] = vmdImporter.ImportAnimation("vmd/player/sneaking.vmd", kmodel);
			anim[5] = vmdImporter.ImportAnimation("vmd/player/idle.vmd", kmodel);
			anim[6] = vmdImporter.ImportAnimation("vmd/player/idle_1.vmd", kmodel);
			anim[7] = vmdImporter.ImportAnimation("vmd/player/idle_2.vmd", kmodel);
			anim[8] = vmdImporter.ImportAnimation("vmd/player/idle_3.vmd", kmodel);

			for (Uint32 i = 0; i < 9; i++) {
				anim[i]->SetRepeat(true);
			}

			kmodel->GetAnimator()->PlayAnimation(anim[0]);

			// Create player entity
			player = world->NewEntity();
			player->SetAABB(&info.aabb);
			player->AttachComponent(Render::GetModelSystem()->NewRiggedModelComponent(mdl_handle, kmodel));
			// Scale visual
			player->GetTransform()->SetScale({ 0.1f, 0.1f, 0.1f });

		}

		void Quit() {
		
			GetWorld()->ClearWorld();

			RG_DELETE_CLASS(GetDefaultAllocator(), LookatCameraController, cam_controller);
			RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);
		
		}

};

int EntryPoint(int argc, String* argv) {
	Application app;
	Initialize(&app);
	Start();
	return 0;
}

rgmain(EntryPoint)

#endif