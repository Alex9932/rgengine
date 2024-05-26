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

using namespace Engine;

static void PSpawnCB_rocket(Particle* particle, ParticleEmitter* emitter) {
    particle->lifetime = 0.5f;
    particle->mul = 1.0f;
    particle->vel = { 0, -1.6f, 0 };
    particle->pos = emitter->GetEntity()->GetTransform()->GetPosition();
}

static void PSpawnCB_explode(Particle* particle, ParticleEmitter* emitter) {
    particle->lifetime = 1;
    particle->mul = 0.99f;

    srand((Uint32)SDL_GetPerformanceCounter());

    vec3 v;

    v.x = (Float32)((rand() % 2000) - 1000) / 1000.0f;
    v.y = (Float32)((rand() % 2000) - 1000) / 1000.0f;
    v.z = (Float32)((rand() % 2000) - 1000) / 1000.0f;

    particle->vel = v.normalize() * 18;
    particle->pos = emitter->GetEntity()->GetTransform()->GetPosition();
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
        FireworkEntityBehavior(ParticleEmitter* r, ParticleEmitter* expl, StreamBuffer* launch_snd, StreamBuffer* expl_snd) {
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

            if (GetUptime() - m_time > 0.025) {
                ParticleEmitter* emitter = ent->GetComponent(Component_PARTICLEEMITTER)->AsParticleEmitter();
                emitter->EmitParticle();
                m_time = GetUptime();
            }
            
            if (pos.y >= 10) { // Explode height

                rgLogInfo(RG_LOG_GAME, "EXPLODE");

                SoundSource* ss = ent->GetComponent(Component_SOUNDSOURCE)->AsSoundSourceComponent();
                ss->SetBuffer(NULL);
                ss->SetBuffer(m_expl_snd);
                ss->Play();

                m_velocity = {};
                m_acceleration = {};
                
                ent->AttachComponent(m_explodeparticle);
                ParticleEmitter* emitter = ent->GetComponent(Component_PARTICLEEMITTER)->AsParticleEmitter();
                for (Uint32 i = 0; i < 128; i++) { // Explode particle count
                    emitter->EmitParticle();
                }
                
                m_time = -1;
            }


        }

        void Launch(const vec3& pos) {
            rgLogInfo(RG_LOG_GAME, "Launching...");
            Entity* ent = GetEntity();
            ent->GetTransform()->SetPosition(pos);
            m_acceleration = {0, 4, 0};
            ent->AttachComponent(m_rocketparticle);
            m_time = GetUptime();
            SoundSource* ss = ent->GetComponent(Component_SOUNDSOURCE)->AsSoundSourceComponent();
            ss->SetBuffer(NULL);
            ss->SetBuffer(m_launch_snd);
            ss->Play();
        }

    private:

        Float64 m_time = -1;

        ParticleEmitter* m_rocketparticle;
        ParticleEmitter* m_explodeparticle;
        StreamBuffer*    m_launch_snd;
        StreamBuffer*    m_expl_snd;

        vec3 m_velocity     = {};
        vec3 m_acceleration = {};

};

static FireworkEntityBehavior* ent_behavior;

static bool EHandler(SDL_Event* event) {
    if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_E) {
        ent_behavior->Launch({0, 0, 0});
    }

    return true;
}

class Application : public BaseGame {

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
            desc.time = 4.33f;
        }
        ~Application() {}

        void MainUpdate() {

            Render::DrawRendererStats();
            Render::DrawProfilerStats();

            // Set light data
            Render::SetGlobalLight(&desc);

            // Update camera
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
            importer.ImportModel("gamedata/models/megumin_obj/doublesided_cape.obj", &objinfo);
            //importer.ImportModel("gamedata/models/cude/untitled.obj", &objinfo);
            //importer.ImportModel("gamedata/sponzaobj/sponza.obj", &objinfo);

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
            fwExplode.lifetime      = 1;
            fwExplode.max_particles = 512;

            fwExplode.sprite_atlas = "platform/textures/xray-nonfree/pfx_sparks.png";
            fwExplode.width  = 4;
            fwExplode.height = 2;
            //fwExplode.sprite_atlas = "platform/textures/pfx_test.png";
            //fwExplode.width  = 4;
            //fwExplode.height = 4;

            ParticleEmitter* pExplodeEmitter = Render::GetParticleSystem()->NewEmitter(&fwExplode);

            ParticleEmitterInfo fwRocket = {};
            fwRocket.spawn_cb      = PSpawnCB_rocket;
            fwRocket.delete_cb     = NULL;
            fwRocket.lifetime      = 0.5f;
            fwRocket.max_particles = 512;

            fwRocket.sprite_atlas = "platform/textures/xray-nonfree/pfx_expl_benzin.png";
            fwRocket.width  = 10;
            fwRocket.height = 10;

            ParticleEmitter* pRocketEmitter = Render::GetParticleSystem()->NewEmitter(&fwRocket);

            //SoundBuffer* lsnd = CreateSoundBuffer("gamedata/sounds/fw_launch.ogg");
            //SoundBuffer* esnd = CreateSoundBuffer("gamedata/sounds/fw_explode.ogg");

            StreamBuffer* lsnd = RG_NEW(StreamBuffer)("gamedata/sounds/fw_launch.ogg");
            StreamBuffer* esnd = RG_NEW(StreamBuffer)("gamedata/sounds/fw_explode.ogg");

            ent_behavior = new FireworkEntityBehavior(pRocketEmitter, pExplodeEmitter, lsnd, esnd);


            SoundSystem* ss = GetSoundSystem();

            ss->SetVolume(1.0f);
            ss->SetCamera(camera);

            SoundSource* source = ss->NewSoundSource();
            source->SetRepeat(false);

            Entity* fw_ent = world->NewEntity();
            fw_ent->SetBehavior(ent_behavior);
            fw_ent->AttachComponent(source);

            RegisterEventHandler(EHandler);

            //sndentl->AttachComponent(emitter);

        }
        
        void Quit() {
            GetWorld()->ClearWorld();

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