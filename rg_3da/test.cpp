#if 1

#include <engine.h>
#include <rgentrypoint.h>

#include <world.h>
#include <camera.h>
#include <freecameracontroller.h>

#include <render.h>
#include <window.h>

#include <modelsystem.h>

#include <objimporter.h>
#include <pm2importer.h>

using namespace Engine;


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
            desc.time = 2.33f;
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