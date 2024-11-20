#if 1
#if 0
//#ifdef _WIN32
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"") 
#endif

#define GAME_DLL
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
#include <particlesystem.h>
#include <kinematicsmodel.h>
#include <pm2importer.h>

#include <animator.h>
#include <animation.h>

#include <logger.h>

#undef min
#undef max
#include <objimporter.h>

#include <soundsystem.h>
#include <rgstb.h>

#include <event.h>

#include <rgthread.h>

#include <rgphysics.h>
#include <phcomponent.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>


using namespace Engine;

//#define GAMEDATA_PATH "gamedata/sponza_old/"

static Camera*      camera;
//static SoundBuffer* soundbuffer;

static Entity* ent0;

static ParticleEmitter* emitter;

static Bool ph_enabled = false;
static bool EHandler(SDL_Event* event) {
#if 0
	if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_E) {
		PlaySoundInfo info = {};
		info.buffer   = soundbuffer;
		info.position = camera->GetTransform()->GetPosition();
		info.volume   = 0.2f;
		info.speed    = 1;
		GetSoundSystem()->PlaySound(&info);
	}
#endif

	if (event->type == SDL_KEYDOWN) {
		switch (event->key.keysym.scancode) {
			case SDL_SCANCODE_P: {
				ph_enabled = !ph_enabled;
				if (ph_enabled) { Engine::GetPhysics()->Enable(); }
				else { Engine::GetPhysics()->Disable(); }
				break;
			}
			case SDL_SCANCODE_R: {
				PHComponent* pcomp = ent0->GetComponent(Component_PH)->AsPHComponent();
				Transform t;
				t.SetPosition({ 0, 5, 0 });
				t.SetRotation({ 0, -0.5f, 1.0f });
				pcomp->SetWorldTransform(&t);
				break;
			}

			case SDL_SCANCODE_E: {
				emitter->EmitParticle();
				break;
			}

			default: { break; }
		}
	}

	return true;
}

static R3D_GlobalLightDescrition desc = {};

static void TaskWorker(void* userdata) {
	KinematicsModel* kmodel = (KinematicsModel*)userdata;

	kmodel->GetAnimator()->Update(GetDeltaTime());
	kmodel->RebuildSkeleton();
	kmodel->SolveCCDIK();
	kmodel->RecalculateTransform();

}

static void PSpawnCB(Particle* particle, ParticleEmitter* emitter, const vec3& pos) {
	vec3 offset = {0, 1.2f, 0};


	particle->lifetime = 3;
	particle->mul = 1.0f;
	particle->vel = { 0, 0, 0 };

	//particle->lifetime = 3;
	//particle->mul = 1.0001f;
	//particle->vel = { 0, 0.3f, 0 };

	particle->pos = emitter->GetEntity()->GetTransform()->GetPosition() + offset;
}

static void PDeleteCB(Particle* particle, ParticleEmitter* emitter) {
	emitter->EmitParticle();
}

class Application : public BaseGame {
	public:

		KinematicsModel* kmodel  = NULL;
		KinematicsModel* kmodel2 = NULL;
		KinematicsModel* kmodel3 = NULL;

		Entity* ent_light0 = NULL;

		World*  world  = NULL;
		FreeCameraController* camcontrol = NULL;

		Animation* anim0 = NULL;
		Animation* anim1 = NULL;
		Animation* anim2 = NULL;

		Application() {
			this->isClient   = true;
			this->isGraphics = true;
			Render::SetRenderFlags(RG_RENDER_FULLSCREEN | RG_RENDER_USE3D);

			desc.color     = { 1, 1, 1 };
			desc.ambient   = 0.45f;
			desc.intensity = 3.3f;
			desc.turbidity = 1.86f;
			desc.time      = 2.33f;

		}
	
		~Application() {
		}

		void ImGuiInputVector3(String label, vec3* vec) {

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

			Render::DrawRendererStats();
			Render::DrawProfilerStats();

			ImGui::Begin("Console");

			ImGui::BeginChild("#Console_txt");
			Sint32 len = (Sint32)Engine::Logger_GetLines();
			for (Sint32 i = len - 1; i >= 0; i--) {
				ImGui::Text("%s", Engine::Logger_GetLine(i));
			}
			ImGui::EndChild();

			static char textbuffer[1024];
			ImGui::InputText(" ", textbuffer, 1024);
			ImGui::SameLine();
			if (ImGui::Button("Send")) {
				rgLogInfo(RG_LOG_GAME, "@ %s", textbuffer);
				SDL_memset(textbuffer, 0, 1024);
			}

			ImGui::End();

			ImGui::Begin("Scene light");
			ImGui::SliderFloat("Time", &desc.time, 0, 6.28);
			ImGui::SliderFloat("Ambient", &desc.ambient, 0, 2);
			ImGui::SliderFloat("Intensity", &desc.intensity, 0, 20);
			ImGui::SliderFloat("Turbidity", &desc.turbidity, 0, 5);
			ImGui::ColorPicker3("Color", desc.color.array);
			ImGui::End();

			Render::SetGlobalLight(&desc);

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

			//ent_light0->GetTransform()->SetPosition({ SDL_sinf(GetUptime() * 0.7) * 9, 1.8, 0 });


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
			Render::SetCamera(&cam);



			R3DUpdateBufferInfo binfo = {};
			binfo.offset = 0;
			binfo.data = kmodel->GetTransforms();
			binfo.handle = kmodel->GetBufferHandle();
			binfo.length = sizeof(mat4) * kmodel->GetBoneCount();
			Render::R3D_UpdateBoneBuffer(&binfo);


			binfo.data = kmodel2->GetTransforms();
			binfo.handle = kmodel2->GetBufferHandle();
			binfo.length = sizeof(mat4) * kmodel2->GetBoneCount();
			Render::R3D_UpdateBoneBuffer(&binfo);

			binfo.data = kmodel3->GetTransforms();
			binfo.handle = kmodel3->GetBufferHandle();
			binfo.length = sizeof(mat4) * kmodel3->GetBoneCount();
			Render::R3D_UpdateBoneBuffer(&binfo);

#if 1

			Task task = {};
			task.proc = TaskWorker;

			task.userdata = kmodel;
			ThreadDispatch(&task);

			task.userdata = kmodel2;
			ThreadDispatch(&task);

			task.userdata = kmodel3;
			ThreadDispatch(&task);
#else

			TaskWorker(kmodel);
			TaskWorker(kmodel2);
			TaskWorker(kmodel3);

#endif

			//kmodel->GetAnimator()->Update(GetDeltaTime());
			//kmodel->RebuildSkeleton();
			//kmodel->SolveCCDIK();
			//kmodel->RecalculateTransform();

			//R3DBoneBufferUpdateInfo binfo = {};
			//binfo.offset = 0;
			//binfo.data   = kmodel->GetTransforms();
			//binfo.handle = kmodel->GetBufferHandle();
			//binfo.length = sizeof(mat4) * kmodel->GetBoneCount();
			//Render::R3D_UpdateBoneBuffer(&binfo);

		}
		
		void Initialize() {

			world = GetWorld();

			camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 1000, rgToRadians(75), 1.777f);
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
			ObjImporter objImporter;
			R3DStaticModelInfo objinfo = {};
			objImporter.ImportModel("gamedata/models/skybox/untitled.obj", &objinfo);

			//pm2Importer.ImportModel("gamedata/models/megumin/v5.pm2", &objinfo);


			R3D_StaticModel* mdl_handle0 = Render::R3D_CreateStaticModel(&objinfo);
			//objImporter.FreeModelData(&objinfo);
			pm2Importer.FreeModelData(&objinfo);

			//pm2Importer.ImportModel("gamedata/models/lamp/lamp.pm2", &objinfo);


#endif
			//R3D_StaticModel* mdl_handle1 = OBJ_ToModel("platform/new/megumin_v4.obj");
			//R3D_StaticModel* mdl_handle1 = OBJ_ToModel("gamedata/sponza_old/sponza.obj");

#if 1

			/*
			pm2Importer.ImportModel("gamedata/models/skybox/geometry.pm2", &pm2info);
			R3D_StaticModel* mdl_handle4 = Render::R3D_CreateStaticModel(&pm2info);
			pm2Importer.FreeModelData(&pm2info);
			*/
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
			String modelname0 = "mmd_models/Rin_Kagamine.pmd";
			String modelname1 = "mmd_models/Miku_Hatsune.pmd";
			String modelname2 = "pmx/gumiv3/GUMI_V3.pmx";
			//String modelname = "pmx/apimiku/Appearance Miku.pmx";

			PMXImporter pmxImporter;
			PMDImporter pmdImporter;

			VMDImporter vmdImporter;

#endif


#if 0

			Entity* ent_test0 = world->NewEntity();
			ent_test0->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));
			ent_test0->GetTransform()->SetPosition({ 0, 0, 0 });
			ent_test0->GetTransform()->SetRotation({ 0, 0, 0 });
			ent_test0->GetTransform()->SetScale({ 1, 1, 1 });

			Entity* ent_test1 = world->NewEntity();
			ent_test1->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));
			ent_test1->GetTransform()->SetPosition({ 1, 0, 0 });
			ent_test1->GetTransform()->SetRotation({ 0, 0, 0 });
			ent_test1->GetTransform()->SetScale({ 1, 1, 1 });

			ent_test1->GetTransform()->SetParent(ent_test0->GetTransform());

#endif

			//ent_light0 = world->NewEntity();
			//Engine::PointLight* lsrc = Render::GetLightSystem()->NewPointLight();
			//lsrc->SetColor({ 1, 0.7f, 0.4f });
			//lsrc->SetIntensity(5);
			//lsrc->SetOffset({ 6, 2.6f, 1 });
			//ent_light0->AttachComponent(lsrc);


			ent0 = world->NewEntity();
			ent0->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle0));

			//Engine::PointLight* l = Render::GetLightSystem()->NewPointLight();
			//l->SetColor({ 1, 0.5, 0.0 });
			//l->SetIntensity(0.3f);
			//l->SetOffset({ -0.84, 1.56, -0.18 });
			//ent0->AttachComponent(l);

			ent0->GetTransform()->SetPosition({ 7.4f, 0, -1.65f });
			//ent0->GetTransform()->SetPosition({ 7.4f, -10, -1.65f });

			ent0->GetTransform()->SetRotation({ 0, -0.8f, 0 });
			//ent0->GetTransform()->SetRotation({ 0, 1.2f, 0 });
			ent0->GetTransform()->SetScale({ 1, 1, 1 });
			//ent0->GetTransform()->SetRotation({ 0, 2.3415f, 0 });
			//ent0->GetTransform()->SetScale({ 0.1f, 0.1f, 0.1f });


			Engine::RGPhysics* ph = Engine::GetPhysics();
			ph->Disable();
			ent0->AttachComponent(ph->NewComponent());
			




			R3DStaticModelInfo pm2info = {};
			pm2Importer.ImportModel("gamedata/sponza/level.pm2", &pm2info);
			//objImporter.ImportModel("F:/c/models/San_Miguel/san-miguel-low-poly.obj", &pm2info);
			//jImporter.ImportModel("D:/models/livingroom/lala.obj", &pm2info);
			R3D_StaticModel* mdl_handle1 = Render::R3D_CreateStaticModel(&pm2info);
			pm2Importer.FreeModelData(&pm2info);
			Entity* ent1 = world->NewEntity();
			ent1->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle1));
			ent1->SetAABB(&pm2info.aabb);
			ent1->GetTransform()->SetPosition({ -1, 0, 0 });
			ent1->GetTransform()->SetRotation({ 0, 0, 0 });
			ent1->GetTransform()->SetScale({ 1, 1, 1 });



			pm2Importer.ImportModel("gamedata/models/table/table.pm2", &objinfo);
			R3D_StaticModel* mdl_handle3 = Render::R3D_CreateStaticModel(&objinfo);
			pm2Importer.FreeModelData(&objinfo);
			Entity* ent3 = world->NewEntity();
			ent3->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle3));
			ent3->SetAABB(&objinfo.aabb);
			ent3->GetTransform()->SetPosition({ 6, 0, 1 });
			ent3->GetTransform()->SetRotation({ 0, -1.05f, 0 });
			ent3->GetTransform()->SetScale({ 1, 1, 1 });

			//Engine::PointLight* l2 = Render::GetLightSystem()->NewPointLight();
			//l2->SetColor({ 1, 0.7f, 0.4f });
			//l2->SetIntensity(5);
			//l2->SetOffset({ 0, 1.6f, 0 });
			//ent3->AttachComponent(l2);


			SoundSystem* ss = GetSoundSystem();

			ss->SetVolume(0.5f);

			ss->SetCamera(camera);
#if 0
			SoundSource* sourcel = ss->NewSoundSource();
			SoundSource* sourcer = ss->NewSoundSource();

			StreamBuffer* sbufferl = RG_NEW(StreamBuffer)("gamedata/sounds/music/GUMI_ChaChaCha_l.ogg");
			StreamBuffer* sbufferr = RG_NEW(StreamBuffer)("gamedata/sounds/music/GUMI_ChaChaCha_r.ogg");

			sourcel->SetBuffer(sbufferl);
			sourcer->SetBuffer(sbufferr);
			sourcel->SetRepeat(true);
			sourcer->SetRepeat(true);

			Entity* sndentl = world->NewEntity();
			Entity* sndentr = world->NewEntity();
			sndentl->AttachComponent(sourcel);
			sndentr->AttachComponent(sourcer);
			sndentl->GetTransform()->SetPosition({ 7.4f, 1, -0.65f });
			sndentr->GetTransform()->SetPosition({ 7.4f, 1,  0.65f });

			//sourcel->Play();
			//sourcer->Play();
#endif

			SoundSource* sourcel = ss->NewSoundSource();
			//StreamBuffer* sbufferl = RG_NEW(StreamBuffer)("gamedata/sounds/music/GUMI_ChaChaCha_l.ogg");
			StreamBuffer* sbufferl = RG_NEW(StreamBuffer)("gamedata/sounds/music/special/freedom_base_radio_1.ogg");

			sourcel->SetBuffer(sbufferl);
			sourcel->SetRepeat(true);

			ParticleEmitterInfo emInfo = {};
			emInfo.spawn_cb  = PSpawnCB;
			emInfo.delete_cb = PDeleteCB;
			emInfo.lifetime  = 3;
			emInfo.max_particles = 256;
#if 0
			emInfo.sprite_atlas  = "platform/textures/pfx_test.png";
			emInfo.width  = 4;
			emInfo.height = 4;
#endif
			
			//emInfo.sprite_atlas  = "platform/textures/xray-nonfree/pfx_expl_benzin.png";
			//emInfo.width     = 10;
			//emInfo.height    = 10;
#if 1
			emInfo.sprite_atlas = "platform/textures/xray-nonfree/pfx_ani-fire01.png";
			emInfo.width  = 11;
			emInfo.height = 7;
#endif

			pm2Importer.ImportModel("gamedata/models/mikufigure/model.pm2", &objinfo);
			R3D_StaticModel* mdl_handle4 = Render::R3D_CreateStaticModel(&objinfo);
			pm2Importer.FreeModelData(&objinfo);

			Entity* sndentl = world->NewEntity();
			sndentl->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle4));
			sndentl->SetAABB(&objinfo.aabb);
			sndentl->AttachComponent(sourcel);
			emitter = Render::GetParticleSystem()->NewEmitter(&emInfo);
			sndentl->AttachComponent(emitter);
			sndentl->GetTransform()->SetPosition({ 6, 0.82f, 1 });
			sndentl->GetTransform()->SetRotation({ 0, -1.05f, 0 });
			sndentl->GetTransform()->SetScale({ 1, 1, 1 });

			sourcel->Play();

#if 0
			//RG_STB_VORBIS sound = RG_STB_vorbis_open_file("C:/Users/alex9932/Desktop/chipi chipi chapa chapa dubi dubi daba daba (looped).ogg", NULL, NULL);
			RG_STB_VORBIS sound = RG_STB_vorbis_open_file("gamedata/sounds/ak47/ak47_shoot1.ogg", NULL, NULL);

			stb_vorbis_info vi = RG_STB_vorbis_get_info(sound.stream);
			SoundBufferCreateInfo sbinfo = {};

			sbinfo.channels = vi.channels;
			sbinfo.samplerate = vi.sample_rate;
			//sbinfo.samples = 
			Uint32 len = RG_STB_vorbis_stream_length_in_samples(sound.stream) * vi.channels;
			sbinfo.samples = len;
			sbinfo.data = rg_malloc(sizeof(Uint16) * len);
			RG_STB_vorbis_get_samples_short_interleaved(sound.stream, vi.channels, (short*)sbinfo.data, len);
			soundbuffer = ss->CreateBuffer(&sbinfo);
			RG_STB_vorbis_close(&sound);
			rg_free(sbinfo.data);
#endif

#if 1
			R3DRiggedModelInfo pmdinfo = {};


			pmdImporter.ImportRiggedModel(modelname0, &pmdinfo);
			R3D_RiggedModel* mdl_handle5 = Render::R3D_CreateRiggedModel(&pmdinfo);
			pmdImporter.FreeRiggedModelData(&pmdinfo);
			kmodel = pmdImporter.ImportKinematicsModel(modelname0);
			Entity* ent5 = world->NewEntity();
			ent5->AttachComponent(Render::GetModelSystem()->NewRiggedModelComponent(mdl_handle5, kmodel));
			ent5->SetAABB(&pmdinfo.aabb);
			ent5->GetTransform()->SetPosition({ 10.5f, 0, -2.2f });
			//ent2->GetTransform()->SetPosition({ 9, -10, -0.4f });
			ent5->GetTransform()->SetRotation({ 0, 1.6f, 0 });
			ent5->GetTransform()->SetScale({ 0.1f, 0.1f, 0.1f });


			pmdImporter.ImportRiggedModel(modelname1, &pmdinfo);
			R3D_RiggedModel* mdl_handle6 = Render::R3D_CreateRiggedModel(&pmdinfo);
			pmdImporter.FreeRiggedModelData(&pmdinfo);
			kmodel2 = pmdImporter.ImportKinematicsModel(modelname1);
			Entity* ent6 = world->NewEntity();
			ent6->AttachComponent(Render::GetModelSystem()->NewRiggedModelComponent(mdl_handle6, kmodel2));
			ent5->SetAABB(&pmdinfo.aabb);
			ent6->GetTransform()->SetPosition({ 10.5f, 0, -0.4f });
			//ent2->GetTransform()->SetPosition({ 9, -10, -0.4f });
			ent6->GetTransform()->SetRotation({ 0, 1.6f, 0 });
			ent6->GetTransform()->SetScale({ 0.1f, 0.1f, 0.1f });

			pmxImporter.ImportRiggedModel(modelname2, &pmdinfo);
			R3D_RiggedModel* mdl_handle7 = Render::R3D_CreateRiggedModel(&pmdinfo);
			pmxImporter.FreeRiggedModelData(&pmdinfo);
			kmodel3 = pmxImporter.ImportKinematicsModel(modelname2);
			Entity* ent7 = world->NewEntity();
			ent7->AttachComponent(Render::GetModelSystem()->NewRiggedModelComponent(mdl_handle7, kmodel3));
			ent5->SetAABB(&pmdinfo.aabb);
			ent7->GetTransform()->SetPosition({ 10.5f, 0, 1.4f });
			//ent2->GetTransform()->SetPosition({ 9, -10, -0.4f });
			ent7->GetTransform()->SetRotation({ 0, 1.6f, 0 });
			ent7->GetTransform()->SetScale({ 0.1f, 0.1f, 0.1f });


			anim0 = vmdImporter.ImportAnimation("vmd/zero_allstar.vmd", kmodel);
			anim1 = vmdImporter.ImportAnimation("vmd/zero_allstar_ph_baked.vmd", kmodel2);
			anim2 = vmdImporter.ImportAnimation("vmd/wavefile_v2.vmd", kmodel3);
			//anim = vmdImporter.ImportAnimation("vmd/wavefile_v2.vmd", kmodel);

			//anim = vmdImporter.ImportAnimation("vmd/zero_allstar.vmd", kmodel);
			anim0->SetRepeat(true);
			anim1->SetRepeat(true);
			anim2->SetRepeat(true);

			kmodel->GetAnimator()->PlayAnimation(anim0);
			kmodel2->GetAnimator()->PlayAnimation(anim1);
			kmodel3->GetAnimator()->PlayAnimation(anim2);
#endif
/*
			Entity* ent4 = world->NewEntity();
			ent4->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle4));
			ent4->GetTransform()->SetPosition({ 0, 0, 0 });
			ent4->GetTransform()->SetRotation({ 0, 0, 0 });
			ent4->GetTransform()->SetScale({ 1, 1, 1 });
*/
			// FJJrVPVfvnmZdAt

			RegisterEventHandler(EHandler);
		}
		
		void Quit() {

			delete anim0;
			delete anim1;
			delete anim2;

			world->ClearWorld();

			RG_DELETE_CLASS(GetDefaultAllocator(), FreeCameraController, camcontrol);
			RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);

		}

		String GetName() { return "rg_3da"; }

	private:

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