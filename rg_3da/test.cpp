#if 1

#include <engine.h>
#include <rgentrypoint.h>

#include <world.h>
#include <camera.h>
#include <freecameracontroller.h>

#include <render.h>
#include <window.h>

#include <event.h>

#include <modelsystem.h>

#include <rgstb.h>

#include <particlesystem.h>
#include <soundsystem.h>
#include <soundsource.h>

#include <objimporter.h>
#include <pm2importer.h>


#include <imgui/ImGuizmo.h>
#include <frustum.h>

using namespace Engine;

static void PSpawnCB_rocket(Particle* particle, ParticleEmitter* emitter, const vec3& pos) {
    particle->lifetime = 0.5f;
    particle->mul = 1.0f;
    particle->vel = { 0, -1.6f, 0 };
    particle->pos = pos; // emitter->GetEntity()->GetTransform()->GetPosition();

    // emitter->GetEntity() is NULL !!!

}

static void PSpawnCB_explode(Particle* particle, ParticleEmitter* emitter, const vec3& pos) {
    particle->lifetime = 1.5f;
    particle->mul = 0.99f;

    srand((Uint32)SDL_GetPerformanceCounter());

    vec3 v;

    v.x = (Float32)((rand() % 2000) - 1000) / 1000.0f;
    v.y = (Float32)((rand() % 2000) - 1000) / 1000.0f;
    v.z = (Float32)((rand() % 2000) - 1000) / 1000.0f;

    particle->vel = v.normalize() * 18;
    particle->pos = pos; // emitter->GetEntity()->GetTransform()->GetPosition();
}

static SoundBuffer* CreateSoundBuffer(String file) {
    SoundSystem* ss = GetSoundSystem();

    RG_STB_VORBIS stream = RG_STB_vorbis_open_file(file, NULL, NULL);
    stb_vorbis_info v_info;
    RG_STB_vorbis_get_info_ptr(stream.stream, &v_info);

    // Get "stream" size

    Uint32 amount = RG_STB_vorbis_stream_length_in_samples(stream.stream);
#if 0
    Uint16 __data[512];
    Sint32 amount = 0;
    while (true) {
        Sint32 decoded = RG_STB_vorbis_get_samples_short_interleaved(stream.stream, v_info.channels, (short*)__data, 512);
        if (decoded <= 0) {
            break;
        }
        amount += decoded;
    }

    
    RG_STB_vorbis_seek_start(stream.stream);
#endif
    Uint16* data_buffer = (Uint16*)rg_malloc(sizeof(Uint16) * amount);
    rgLogInfo(RG_LOG_GAME, "OGG: %d %d %d", v_info.channels, v_info.sample_rate, amount);

    // Read all data
    RG_STB_vorbis_get_samples_short_interleaved(stream.stream, v_info.channels, (short*)data_buffer, amount);

//    for (Uint32 i = 0; i < amount; i++) {
//        data_buffer[i] = (Uint16)(rand() % 65535);
//    }

    // Create buffer
    SoundBufferCreateInfo info = {};
    info.channels   = v_info.channels;
    info.samplerate = v_info.sample_rate;
    info.samples    = amount;
    info.data       = data_buffer;

    SoundBuffer* buffer = ss->CreateBuffer(&info);

    // Clean up
    RG_STB_vorbis_close(&stream);
    rg_free(data_buffer);

    return buffer;
}

class FireworkEntityBehavior : public EntityBehavior {
    public:
        FireworkEntityBehavior(ParticleEmitter* r, ParticleEmitter* expl, SoundBuffer* launch_snd, SoundBuffer* expl_snd) {
            m_rocketparticle  = r;
            m_explodeparticle = expl;

            m_launch_snd = launch_snd;
            m_expl_snd   = expl_snd;
        }
        ~FireworkEntityBehavior() {}

        void Update(Float64 dt) {
            if (m_time < 0) { return; }

            Entity* ent = GetEntity();
            Transform* t = ent->GetTransform();

            m_velocity += m_acceleration * dt;
            vec3 pos = t->GetPosition();

            pos += m_velocity * dt;

            t->SetPosition(pos);

            //rgLogInfo(RG_LOG_GAME, "Position: %f %f %f", pos.x, pos.y, pos.z);

            vec3 w_pos = GetEntity()->GetTransform()->GetWorldPosition();

            if (GetUptime() - m_time > 0.025) {
                m_rocketparticle->EmitParticle(w_pos);
                m_time = GetUptime();
            }
            
            if (pos.y >= m_explodeHeight) { // Explode height

                //rgLogInfo(RG_LOG_GAME, "EXPLODE");

                PlaySoundInfo snd_info = {};
                snd_info.position = w_pos;
                snd_info.volume   = 10;
                snd_info.speed    = 1;
                snd_info.buffer   = m_expl_snd;
                GetSoundSystem()->PlaySound(&snd_info);

                m_velocity = {};
                m_acceleration = {};
                
                for (Uint32 i = 0; i < 128; i++) { // Explode particle count
                    m_explodeparticle->EmitParticle(w_pos);
                }
                
                m_time = -1;

                // Delete "rocket"
                GetWorld()->FreeEntity(GetEntity());

                // TODO: Rewrite this
                delete this;
            }


        }

        void Launch(const vec3& pos) {
            //rgLogInfo(RG_LOG_GAME, "Launching...");
            Entity* ent = GetEntity();
            ent->GetTransform()->SetPosition(pos);

            m_explodeHeight = 20 + ((Float32)(rand() % 1000) / 1000.0f) * 6;

            vec3 v;
            v.x = (((Float32)(rand() % 1000) / 1000.0f) * 6) - 3;
            v.y = 12;
            v.z = (((Float32)(rand() % 1000) / 1000.0f) * 6) - 3;

            m_acceleration = v;
            m_time = GetUptime();

            PlaySoundInfo snd_info = {};
            snd_info.position = ent->GetTransform()->GetWorldPosition();
            snd_info.volume   = 3;
            snd_info.speed    = 1;
            snd_info.buffer   = m_launch_snd;
            GetSoundSystem()->PlaySound(&snd_info);
        }

    private:

        Float64 m_time = -1;

        Float32 m_explodeHeight = 0;

        ParticleEmitter* m_rocketparticle;
        ParticleEmitter* m_explodeparticle;
        SoundBuffer*     m_launch_snd;
        SoundBuffer*     m_expl_snd;

        vec3 m_velocity     = {};
        vec3 m_acceleration = {};

};

static ParticleEmitter* pExplodeEmitter;
static ParticleEmitter* pRocketEmitter;
static SoundBuffer* lsnd;
static SoundBuffer* esnd;

static void LaunchFWRocket(const vec3& pos) {
    // TODO: Rewrite this
    FireworkEntityBehavior* ent_behavior = new FireworkEntityBehavior(pRocketEmitter, pExplodeEmitter, lsnd, esnd);

    World* world = GetWorld();
    Entity* fw_ent = world->NewEntity();
    fw_ent->SetBehavior(ent_behavior);
    ent_behavior->Launch({ 0, 0, 0 });
}

static bool EHandler(SDL_Event* event) {
    if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_E) {
        LaunchFWRocket({ 0, 0, 0 });
    }

    return true;
}

static vec4 sphere = {0, 0, 0, 0.1f};

static AABB aabb = { { 0, 0, 0 }, { 1, 1, 1 } };

static void DrawImGui(Camera* camera) {
    ImGuizmo::BeginFrame();
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    mat4 view  = {};
    mat4 model = {};
    vec3 position = camera->GetTransform()->GetPosition();
    vec3 rotation = camera->GetTransform()->GetRotation();
    mat4_view(&view, position, rotation);
    mat4_model(&model, sphere.xyz, { 0, 0, 0 }, { 1, 1, 1 });
    ImGuizmo::Manipulate(view.m, camera->GetProjection()->m, ImGuizmo::TRANSLATE, ImGuizmo::LOCAL, model.m);
    vec3 newpos = {};
    mat4_decompose(&newpos, NULL, NULL, model);
    sphere.xyz = newpos;

}

class Application : public BaseGame {

    Frustum frustum;
    Camera* camera = NULL;
    FreeCameraController* camcontrol = NULL;

    R3D_GlobalLightDescrition desc = {};

    public:
        Application() {
            isClient   = true;
            isGraphics = true;

            Render::SetRenderFlags(RG_RENDER_FULLSCREEN | RG_RENDER_USE3D);

            // Setup light
            desc.color = { 1, 1, 1 };
            desc.ambient = 0.45f;
            desc.intensity = 3.3f;
            desc.turbidity = 1.86f;
            desc.time = 2.33f;
        }
        ~Application() {}

        void MainUpdate() {


            Render::DrawRendererStats();
            Render::DrawProfilerStats();


            DrawImGui(camera);

            // Set light data
            Render::SetGlobalLight(&desc);

            // Update camera
            ivec2 size = {};
            Engine::GetWindowSize(&size);
            camera->SetAspect((Float32)size.x / (Float32)size.y);
            camera->ReaclculateProjection();

            camcontrol->Update();
            camera->Update(GetDeltaTime());

            mat4 camera_view;
            vec3 cam_position = camera->GetTransform()->GetPosition();
            vec3 cam_rotation = camera->GetTransform()->GetRotation();
            mat4_view(&camera_view, cam_position, cam_rotation);

            CreateFrustumInfo finfo = {};
            finfo.result = &frustum;
            finfo.proj = camera->GetProjection();
            finfo.view = &camera_view;
            CreateFrustum(&finfo);

            ImGui::Begin("Frustum Cull");
            //Bool inFrustum = SphereInFrustum(&frustum, sphere.xyz, sphere.w);
            Bool inFrustum = AABBInFrustum(&frustum, aabb);
            ImGui::InputFloat3("Position", sphere.xyz.array, "%.3f", ImGuiInputTextFlags_ReadOnly);
            if (inFrustum) {
                ImGui::Text("In frustum");
            }
            ImGui::End();

            R3D_CameraInfo cam = {};
            cam.projection = *camera->GetProjection();
            cam.position = camera->GetTransform()->GetPosition();
            cam.rotation = camera->GetTransform()->GetRotation();
            Render::R3D_SetCamera(&cam);

        }
        
        void Initialize() {

            World* world = GetWorld();

            // Create camera

            camera = RG_NEW_CLASS(GetDefaultAllocator(), Camera)(world, 0.1f, 1000, rgToRadians(75), 1.777f);
            camera->GetTransform()->SetPosition({ 0, 1.3f, 2 });
            camera->GetTransform()->SetRotation({ 0, 0, 0 });
            camcontrol = RG_NEW_CLASS(GetDefaultAllocator(), FreeCameraController)(camera);

            //// Create object

            // Load geometry
            R3DStaticModelInfo objinfo = {};

            ObjImporter importer;
            importer.ImportModel("gamedata/models/meg/untitled2.obj", &objinfo);
            //importer.ImportModel("gamedata/models/megumin_obj/doublesided_cape.obj", &objinfo);
            //importer.ImportModel("gamedata/models/cude/untitled.obj", &objinfo);
            //importer.ImportModel("gamedata/sponzaobj/sponza.obj", &objinfo);

            aabb = objinfo.aabb;

            //PM2Importer importer;
            //importer.ImportModel("gamedata/models/megumin/v5.pm2", &objinfo);


            // Create handle
            R3D_StaticModel* mdl_handle = Render::R3D_CreateStaticModel(&objinfo);
            importer.FreeModelData(&objinfo);

            // Create entity
            Entity* ent = world->NewEntity();
            ent->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle));
            //ent->GetTransform()->SetScale({ 0.01f, 0.01f, 0.01f });


            // Forework
            ParticleEmitterInfo fwExplode = {};
            fwExplode.spawn_cb      = PSpawnCB_explode;
            fwExplode.delete_cb     = NULL;
            fwExplode.lifetime      = 1.5f;
            fwExplode.max_particles = 4096;
            fwExplode.gravity       = -9.81f;

            fwExplode.sprite_atlas = "platform/textures/xray-nonfree/pfx_sparks.png";
            fwExplode.width  = 4;
            fwExplode.height = 2;
            //fwExplode.sprite_atlas = "platform/textures/pfx_test.png";
            //fwExplode.width  = 4;
            //fwExplode.height = 4;

            pExplodeEmitter = Render::GetParticleSystem()->NewEmitter(&fwExplode);

            ParticleEmitterInfo fwRocket = {};
            fwRocket.spawn_cb      = PSpawnCB_rocket;
            fwRocket.delete_cb     = NULL;
            fwRocket.lifetime      = 0.5f;
            fwRocket.max_particles = 4096;
            fwRocket.gravity       = 0;

            //fwRocket.sprite_atlas = "platform/textures/xray-nonfree/pfx_spark_01.png";
            //fwRocket.width  = 1;
            //fwRocket.height = 1;

            fwRocket.sprite_atlas = "platform/textures/xray-nonfree/pfx_sparks.png";
            fwRocket.width = 4;
            fwRocket.height = 2;

            pRocketEmitter = Render::GetParticleSystem()->NewEmitter(&fwRocket);

            lsnd = CreateSoundBuffer("gamedata/sounds/fw_launch.ogg");
            esnd = CreateSoundBuffer("gamedata/sounds/fw_explode.ogg");

            SoundSystem* ss = GetSoundSystem();

            ss->SetVolume(1.0f);
            ss->SetCamera(camera);

            RegisterEventHandler(EHandler);

        }
        
        void Quit() {
            GetWorld()->ClearWorld();

            Render::GetParticleSystem()->DeleteEmitter(pExplodeEmitter);
            Render::GetParticleSystem()->DeleteEmitter(pRocketEmitter);

            GetSoundSystem()->DestroyBuffer(lsnd);
            GetSoundSystem()->DestroyBuffer(esnd);

            RG_DELETE_CLASS(GetDefaultAllocator(), FreeCameraController, camcontrol);
            RG_DELETE_CLASS(GetDefaultAllocator(), Camera, camera);
        }

        String GetName() {
            return "rg_3da";
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