#if 0
//#ifdef _WIN32
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"") 
#endif

#include <rgentrypoint.h>

#include <stdio.h>
#include <engine.h>
#include <rgmath.h>

#include <pmd.h>

//#include "obj_loader.h"
#include <mmdimporter.h>
#include "ksmimporter.h"

#include <window.h>
#include <render.h>
#include <world.h>
#include <camera.h>
#include <freecameracontroller.h>
#include <allocator.h>
#include <filesystem.h>
#include <modelsystem.h>
#include <lightsystem.h>
#include <kinematicsmodel.h>
#include <pm2importer.h>

#include <animator.h>
#include <animation.h>

#undef min
#undef max
#include <objimporter.h>


#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>


using namespace Engine;

//#define GAMEDATA_PATH "gamedata/sponza_old/"

static void RecalculateTangetns(Uint32 vCount, R3D_Vertex* vertices, Uint32 startidx, Uint32 iCount, Uint32* indices) {

	// Calculate tangents
	for (Uint32 i = 0; i < iCount / 3; i += 3) {
		Uint32 v0idx = indices[startidx + i + 0];
		Uint32 v1idx = indices[startidx + i + 1];
		Uint32 v2idx = indices[startidx + i + 2];
		R3D_Vertex* v0 = &vertices[v0idx];
		R3D_Vertex* v1 = &vertices[v1idx];
		R3D_Vertex* v2 = &vertices[v2idx];
		Float32 dx1 = v1->pos.x - v0->pos.x;
		Float32 dy1 = v1->pos.y - v0->pos.y;
		Float32 dz1 = v1->pos.z - v0->pos.z;
		Float32 dx2 = v2->pos.x - v0->pos.x;
		Float32 dy2 = v2->pos.y - v0->pos.y;
		Float32 dz2 = v2->pos.z - v0->pos.z;
		Float32 du1 = v1->uv.x - v0->uv.x;
		Float32 dv1 = v1->uv.y - v0->uv.y;
		Float32 du2 = v2->uv.x - v0->uv.x;
		Float32 dv2 = v2->uv.y - v0->uv.y;
		Float32 r = 1.0f / (du1 * dv2 - dv1 * du2);
		dx1 *= dv2;
		dy1 *= dv2;
		dz1 *= dv2;
		dx2 *= dv1;
		dy2 *= dv1;
		dz2 *= dv1;
		Float32 tx = (dx1 - dx2) * r;
		Float32 ty = (dy1 - dy2) * r;
		Float32 tz = (dz1 - dz2) * r;
		v0->tang.x = tx;
		v0->tang.y = ty;
		v0->tang.z = tz;
		v1->tang = v0->tang;
		v2->tang = v0->tang;
	}

}

class Application : public BaseGame {
	public:

		Entity* ent_test0;
		Entity* ent_test1;

		Entity* ent_light0;

		Entity* ent0 = NULL;
		Entity* ent1 = NULL;
		Entity* ent2 = NULL;

		World*  world  = NULL;
		Camera* camera = NULL;
		FreeCameraController* camcontrol = NULL;

		Animation* anim;



		Application() {
			this->isClient   = true;
			this->isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_FULLSCREEN | RG_RENDER_USE3D);
		}
	
		~Application() {
		}

		void ImGuiInputVector3(String label, vec3* vec) {

			//float ItemSpacing = ImGui::GetStyle().ItemSpacing.x;
			//float pos = ItemSpacing;

			//ImGui::SameLine(pos);
			//ImGui::InputFloat("##X", &vec->x);
			
			//pos += ImGui::GetItemRectSize().x + ItemSpacing;

			//ImGui::SameLine(100);
			//ImGui::InputFloat("##Y", &vec->y);
			//pos += ImGui::GetItemRectSize().x + ItemSpacing;

			
			//ImGui::SameLine(pos);
			//ImGui::InputFloat("Z", &vec->z);
			//pos += ImGui::GetItemRectSize().x + ItemSpacing;

			ImGui::PushItemWidth(50);

			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::InputFloat("##X", &vec->x);
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
			ImGui::InputFloat("##Y", &vec->y);
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 255, 255));
			ImGui::InputFloat("##Z", &vec->z);
			ImGui::PopStyleColor();

			ImGui::SameLine();
			ImGui::Text(label);

			ImGui::PopItemWidth();

		}

		void MainUpdate() {

			RenderInfo renderer_info = {};
			Render::GetInfo(&renderer_info);

			ImGui::Begin("Renderer stats");

#if 0
			static float colors[4] = {};
			Engine::PointLight* pl = ent0->GetComponent(Component_POINTLIGHT)->AsPointLightComponent();
			ImGui::ColorPicker3("Light color", pl->GetColor().array);
#endif

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


			ImGui::Begin("Camera");
			vec3 pos = camera->GetTransform()->GetPosition();
			vec3 rot = camera->GetTransform()->GetRotation();

			ImGuiInputVector3("Position", &pos);
			ImGuiInputVector3("Rotation", &rot);

			
			ImGui::End();


#if 0
			ent_test0->GetTransform()->SetRotation({ 0, (Float32)GetUptime(), 0});
			ent_test1->GetTransform()->SetRotation({ 0, (Float32)GetUptime() * 0.5f, 0 });
#endif		

			ent_light0->GetTransform()->SetPosition({ SDL_sinf(GetUptime() * 0.7) * 9, 1.8, 0 });


			ivec2 size = {};
			Engine::GetWindowSize(&size);
			camera->SetAspect((Float32)size.x / (Float32)size.y);
			camera->ReaclculateProjection();

			camcontrol->Update();
			camera->Update(GetDeltaTime());

			R3D_CameraInfo cam = {};
			cam.projection = *camera->GetProjection();
			cam.position = camera->GetTransform()->GetPosition();
			cam.rotation = camera->GetTransform()->GetRotation();
			Render::R3D_SetCamera(&cam);

			R3D_PushModelInfo info = {};

#if 0
			info.handle = ent_test0->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle();
			info.matrix = *ent_test0->GetTransform()->GetMatrix();
			Render::R3D_PushModel(&info);

			info.handle = ent_test1->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle();
			info.matrix = *ent_test1->GetTransform()->GetMatrix();
			Render::R3D_PushModel(&info);
#endif

			info.handle = ent0->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle();

			info.matrix = *ent0->GetTransform()->GetMatrix();
			//mat4_model(&info.matrix, { 7.4, 0, -1.65 }, { 0, -0.8f, 0 }, { 1, 1, 1 });
			
			//mat4_model(&info.matrix, { 1, 0, 0 }, { 0, (Float32)GetUptime() * 0.8f, 0 }, { 0.1f, 0.1f, 0.1f });
			Render::R3D_PushModel(&info);

			info.handle = ent1->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle();
			//mat4_model(&info.matrix, { -1, 0, 0 }, { 0, 0, 0 }, { 0.01f, 0.01f, 0.01f });

			info.matrix = *ent1->GetTransform()->GetMatrix();
			//mat4_model(&info.matrix, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 });
			Render::R3D_PushModel(&info);

			Engine::RiggedModelComponent* rmdl = ent2->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();


			//R3DBoneBufferUpdateInfo
			KinematicsModel* kmodel = rmdl->GetKinematicsModel();

			kmodel->GetAnimator()->Update(GetDeltaTime());
			kmodel->RebuildSkeleton();
			kmodel->SolveCCDIK();
			kmodel->RecalculateTransform();


			R3DBoneBufferUpdateInfo binfo = {};
			binfo.offset = 0;
			binfo.data   = kmodel->GetTransforms();
			binfo.handle = kmodel->GetBufferHandle();
			binfo.length = sizeof(mat4) * kmodel->GetBoneCount();
			Render::R3D_UpdateBoneBuffer(&binfo);



			info.handle = rmdl->GetHandle();
			info.handle_bonebuffer = rmdl->GetKinematicsModel()->GetBufferHandle();
			//mat4_model(&info.matrix, { 9, 0, -0.4 }, { 0, 1.6, 0 }, { 0.1, 0.1, 0.1 });
			info.matrix = *ent2->GetTransform()->GetMatrix();
			Render::R3D_PushModel(&info);

		}
		
		void Initialize() {

			//world  = new World();
			//camera = new Camera(world, 0.1f, 1000, rgToRadians(75), 1.777f);
			world = GetWorld();

			camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 100, rgToRadians(75), 1.777f);
			camera->GetTransform()->SetPosition({5.16, 1.49, 0.1});
			//camera->GetTransform()->SetRotation({0, 3.1415 / 2, 0});
			camera->GetTransform()->SetRotation({ 0.11, 1.28, 0 });
			camcontrol = RG_NEW_CLASS(GetDefaultAllocator(), FreeCameraController)(camera);

			//pmd_file* pmd = pmd_load("mmd_models/Rin_Kagamine.pmd");
			//pmd_file* pmd = pmd_load("mmd_models/Rin_Kagamene_act2.pmd");
			//pmd_file* pmd = pmd_load("mmd_models/Miku_Hatsune.pmd");
#if 0
			PMDImporter pmdImporter;
			R3DStaticModelInfo pmdinfo = {};
			pmdImporter.ImportModel("mmd_models/Rin_Kagamine.pmd", &pmdinfo);
			R3D_StaticModel* mdl_handle0 = Render::R3D_CreateStaticModel(&pmdinfo);
			pmdImporter.FreeModelData(&pmdinfo);
#else
			PM2Importer pm2Importer;
			//ObjImporter objImporter;
			R3DStaticModelInfo objinfo = {};
			//objImporter.ImportModel("gamedata/models/megumin/megumin_v4.obj", &objinfo);
			pm2Importer.ImportModel("gamedata/models/megumin/v4.pm2", &objinfo);
			R3D_StaticModel* mdl_handle0 = Render::R3D_CreateStaticModel(&objinfo);
			//objImporter.FreeModelData(&objinfo);
			pm2Importer.FreeModelData(&objinfo);
#endif
			//R3D_StaticModel* mdl_handle1 = OBJ_ToModel("platform/new/megumin_v4.obj");
			//R3D_StaticModel* mdl_handle1 = OBJ_ToModel("gamedata/sponza_old/sponza.obj");

#if 1
			R3DStaticModelInfo pm2info = {};
			pm2Importer.ImportModel("gamedata/sponza/level.pm2", &pm2info);
			R3D_StaticModel* mdl_handle1 = Render::R3D_CreateStaticModel(&pm2info);
			pm2Importer.FreeModelData(&pm2info);
#endif
#if 0
			ObjImporter objImporter;
			R3DStaticModelInfo objinfo = {};
			objImporter.ImportModel("gamedata/sponza_old/sponza2.obj", &objinfo);
			R3D_StaticModel* mdl_handle1 = Render::R3D_CreateStaticModel(&objinfo);
			objImporter.FreeModelData(&objinfo);
#endif
#if 0
			KSMImporter ksmImporter("gamedata/ksm");
			R3DStaticModelInfo ksminfo = {};
			ksmImporter.ImportModel("sponza", &ksminfo);
			R3D_StaticModel* mdl_handle1 = Render::R3D_CreateStaticModel(&ksminfo);
			ksmImporter.FreeModelData(&ksminfo);
#endif
			
#if 1
			//String modelname = "mmd_models/Rin_Kagamine.pmd";
			String modelname = "mmd_models/Miku_Hatsune.pmd";

			PMDImporter pmdImporter;
			R3DRiggedModelInfo pmdinfo = {};
			pmdImporter.ImportRiggedModel(modelname, &pmdinfo);
			R3D_RiggedModel* mdl_handle2 = Render::R3D_CreateRiggedModel(&pmdinfo);
			pmdImporter.FreeRiggedModelData(&pmdinfo);
			KinematicsModel* kmodel = pmdImporter.ImportKinematicsModel(modelname);

			VMDImporter vmdImporter;
			anim = vmdImporter.ImportAnimation("vmd/wavefile_v2.vmd", kmodel);
			anim->SetRepeat(true);

			kmodel->GetAnimator()->PlayAnimation(anim);

#endif


#if 0

			ent_test0 = world->NewEntity();
			ent_test0->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));
			ent_test0->GetTransform()->SetPosition({ 0, 0, 0 });
			ent_test0->GetTransform()->SetRotation({ 0, 0, 0 });
			ent_test0->GetTransform()->SetScale({ 1, 1, 1 });

			ent_test1 = world->NewEntity();
			ent_test1->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));
			ent_test1->GetTransform()->SetPosition({ 1, 0, 0 });
			ent_test1->GetTransform()->SetRotation({ 0, 0, 0 });
			ent_test1->GetTransform()->SetScale({ 1, 1, 1 });

			ent_test1->GetTransform()->SetParent(ent_test0->GetTransform());

#endif

			ent_light0 = world->NewEntity();
			Engine::PointLight* lsrc = Render::GetLightSystem()->NewPointLight();
			lsrc->SetColor({ 1, 0.7f, 0.4f });
			lsrc->SetIntensity(5);
			lsrc->SetOffset({ 0, 0, 0 });
			ent_light0->AttachComponent(lsrc);


			ent0 = world->NewEntity();
			ent0->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));

			Engine::PointLight* l = Render::GetLightSystem()->NewPointLight();
			l->SetColor({ 1, 0.5, 0.0 });
			l->SetIntensity(2);
			l->SetOffset({ -0.86, 1.56, -0.21 });
			ent0->AttachComponent(l);

			ent0->GetTransform()->SetPosition({ 7.4f, 0, -1.65f });

			ent0->GetTransform()->SetRotation({ 0, -0.8f, 0 });
			ent0->GetTransform()->SetScale({ 1, 1, 1 });
			//ent0->GetTransform()->Recalculate();

			ent1 = world->NewEntity();
			ent1->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle1));

			Engine::PointLight* l2 = Render::GetLightSystem()->NewPointLight();
			l2->SetColor({ 1, 0.9f, 0.8f });
			l2->SetIntensity(10);
			l2->SetOffset({6, 1, 0});
			ent1->AttachComponent(l2);

			ent1->GetTransform()->SetPosition({ -1, 0, 0 });
			ent1->GetTransform()->SetRotation({ 0, 0, 0 });
			ent1->GetTransform()->SetScale({ 1, 1, 1 });
			//ent1->GetTransform()->Recalculate();
#if 1
			ent2 = world->NewEntity();
			ent2->AttachComponent(Render::GetModelSystem()->NewRiggedModelComponent(mdl_handle2, kmodel));
			ent2->GetTransform()->SetPosition({ 9, 0, -0.4f });
			ent2->GetTransform()->SetRotation({ 0, 1.6f, 0 });
			ent2->GetTransform()->SetScale({ 0.1f, 0.1f, 0.1f });
			//ent2->GetTransform()->Recalculate();
#endif
		}
		
		void Quit() {

			delete anim;

			Render::R3D_DestroyStaticModel(ent0->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle());
			Render::R3D_DestroyStaticModel(ent1->GetComponent(Component_MODELCOMPONENT)->AsModelComponent()->GetHandle());
			Render::R3D_DestroyRiggedModel(ent2->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent()->GetHandle());

#if 0
			world->FreeEntity(ent_test0);
			world->FreeEntity(ent_test1);
#endif

			world->FreeEntity(ent0);
			world->FreeEntity(ent1);
			world->FreeEntity(ent2);

			RG_DELETE_CLASS(GetDefaultAllocator(), FreeCameraController, camcontrol);
			RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);

		}

		String GetName() { return "rg_3da"; }

	private:

};

int EntryPoint(int argc, String* argv) {
	Application app;
	Initialize(&app);
	Start();
	return 0;
}

rgmain(EntryPoint)