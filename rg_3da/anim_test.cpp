#if 1

#define GAME_DLL
#include <rgentrypoint.h>

#include <engine.h>

#include <render.h>
#include <rimgui.h>
#include <window.h>
#include <modelsystem.h>

#include <world.h>
#include <camera.h>
#include <kinematicsmodel.h>
#include <lookatcameracontroller.h>
#include <freecameracontroller.h>

#include <event.h>
#include <input.h>

#include <animimporter.h>

#include <rmodelmanager.h>

using namespace Engine;


static Camera*                 camera         = NULL;
static FreeCameraController*   cam_controller = NULL;
//static LookatCameraController* cam_controller = NULL;

static Entity*                 player         = NULL;

static R3D_GlobalLightDescrition desc = {};


// Animations
static Animation* anim[9] = {};

static int val = 0;
static const char* items[] = { "None", "Stand", "Walk", "Run", "Squat", "Sneaking", "Idle", "Do Idle 1" , "Do Idle 2" , "Do Idle 3" };


static Bool Handler(SDL_Event* event) {

	//if () {

	//}

	return true;

}

void DrawImGuiCallback() {

	ImGui::Begin("Scene light");
	ImGui::SliderFloat("Time", &desc.time, 0, 6.28);
	ImGui::SliderFloat("Ambient", &desc.ambient, 0, 2);
	ImGui::SliderFloat("Intensity", &desc.intensity, 0, 20);
	ImGui::SliderFloat("Turbidity", &desc.turbidity, 0, 5);
	ImGui::ColorPicker3("Color", desc.color.array);
	ImGui::End();


	ImGui::Begin("Animation control");


	RiggedModelComponent* rmc = player->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();
	KinematicsModel* kmodel = rmc->GetKinematicsModel();

	if (ImGui::Combo("Animation", &val, items, IM_ARRAYSIZE(items))) {
		if (val == 0) {
			kmodel->GetAnimator()->PlayAnimation(NULL);
		}
		else {
			Animation* animation = anim[val - 1];
			animation->SetSpeed(1);
			kmodel->GetAnimator()->PlayAnimation(animation);
		}
	}

	Animation* anim = kmodel->GetAnimator()->GetCurrentAnimation();
	Float32 speed = 1;
	if (anim) {
		speed = (Float32)anim->GetSpeed();
	}
	if (ImGui::SliderFloat("Animation speed", &speed, 0, 10)) {
		if (anim) { anim->SetSpeed(speed); }
	}

	ImGui::End();
}

class Application : public BaseGame {
	public:
		Application()  {
			isClient = true;
			isGraphics = true;
			Render::SetRenderFlags(0);

			desc.color = { 1, 1, 1 };
			desc.ambient = 0.45f;
			desc.intensity = 3.3f;
			desc.turbidity = 1.86f;
			desc.time = 2.33f;
		}

		~Application() {}

		void MainUpdate() {

			if (IsKeyDown(SDL_SCANCODE_W)) {

			}


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
		
			RiggedModelComponent* rmc = player->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();
			KinematicsModel* km = rmc->GetKinematicsModel();
			// Calculate skeleton
			km->GetAnimator()->Update(GetDeltaTime());
			km->RebuildSkeleton();
			km->SolveCCDIK();
			km->RecalculateTransform();

			R3DUpdateBufferInfo binfo = {};
			binfo.offset = 0;
			binfo.data   = km->GetTransforms();
			binfo.handle = rmc->GeBoneBuffer();
			binfo.length = sizeof(mat4) * km->GetBoneCount();
			Render::UpdateBoneBuffer(&binfo);

			//vec3 camera_offset = { 0, 1.67f, 0 };
			//vec3 camera_pos = player->GetTransform()->GetWorldPosition() + camera_offset;
			//cam_controller->SetLookAtPosition(&camera_offset);

		}

		void Initialize() {
		
			Render::RegisterImGuiDrawCallback(DrawImGuiCallback);

			World* world = GetWorld();

			// Create 3-rd person camera
			camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 1000, rgToRadians(75), 1.777f);

			//cam_controller = RG_NEW_CLASS(GetDefaultAllocator(), LookatCameraController)(camera);

			cam_controller = RG_NEW_CLASS(GetDefaultAllocator(), FreeCameraController)(camera);
			//cam_controller->SetAngles({ 0, 3.1415, 0 });
			camera->GetTransform()->SetPosition({ 0.0f, 1.6f, 2.0f });

			AnimImporter pm2anim;

			// Load geometry


			// Create player entity
			player = world->NewEntity();
			//player->SetAABB(&info.aabb);
			player->AttachComponent(GetModelSystem()->NewRiggedModelComponent("Gilberta"));
			// Scale visual
			//player->GetTransform()->SetScale({ 0.1f, 0.1f, 0.1f });
			player->GetTransform()->SetScale({ 1.5f, 1.5f, 1.5f });

			// Load animations
			anim[0] = pm2anim.ImportAnimation("gamedata/anims/fluorite-stand.anim");
			anim[1] = pm2anim.ImportAnimation("gamedata/anims/fluorite-walk.anim");
			anim[2] = pm2anim.ImportAnimation("gamedata/anims/fluorite-capoeira.anim");

			for (Uint32 i = 0; i <3; i++) {
				anim[i]->SetRepeat(true);
			}

			//kmodel->GetAnimator()->PlayAnimation(anim[0]);



			// Level ground
#if 0
			R3DStaticModelInfo sinfo = {};
			ImportModelInfo iminfo2 = {};
			iminfo2.path = "gamedata/models";
			iminfo2.file = "flatplane.pm2";
			//iminfo2.file = "Sponza.pm2";
			//iminfo2.file = "NewSponza_Main_glTF_003.pm2";
			//iminfo2.file = "table_drugoy.pm2";
			iminfo2.info.as_static = &sinfo;
			pm2.ImportModel(&iminfo2);
			R3D_StaticModel* level_mdl_handle = Render::CreateStaticModel(&sinfo);

			FreeModelInfo fminfo = {};
			fminfo.info.as_static = &sinfo;
			pm2.FreeModelData(&fminfo);
#endif
			//R3D_StaticModel* level_mdl_handle = GetStaticModel("Sponza");
			R3D_StaticModel* level_mdl_handle = GetStaticModel("NewSponza_Main_glTF_003");
			//R3D_StaticModel* level_mdl_handle = GetStaticModel("flatplane");
			mat4 model = MAT4_IDENTITY();
			AABB aabb = { { -99999, -99999, -99999 }, { 99999, 99999, 99999 } };
			world->NewStatic(level_mdl_handle, &model, &aabb);

			Float32 ox = -6;
			Float32 oz = -15;
			for (Uint32 i = 0; i < 49; i++) {
				R3D_StaticModel* level_mdl_handle = GetStaticModel("table_drugoy");
				mat4 model = MAT4_IDENTITY();
				Float32 x = ox + (Float32)(i % 7) * 2;
				Float32 z = oz + (Float32)(i / 7) * 2;
				Float32 rx = 0;// rgRandFloat() * 2 * RG_PI;
				Float32 ry = rgRandFloat() * 2 * RG_PI;
				Float32 rz = 0;// rgRandFloat() * 2 * RG_PI;
				mat4_model(&model, { x, 0, z }, { rx, ry, rz }, { 1, 1, 1 });
				aabb = { { x, 0, z }, { x + 2, 2, z + 2 } };
				world->NewStatic(level_mdl_handle, &model, &aabb);
			}

		}

		void Quit() {
		
			GetWorld()->ClearWorld();


			for (Uint32 i = 0; i < 3; i++) {
				RG_DELETE(Animation, anim[i]);
			}

			//RG_DELETE_CLASS(GetDefaultAllocator(), LookatCameraController, cam_controller);
			RG_DELETE_CLASS(GetDefaultAllocator(), FreeCameraController, cam_controller);
			RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);
		
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

#endif