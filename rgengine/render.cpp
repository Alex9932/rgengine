#define DLL_EXPORT
#include "render.h"
#include "window.h"
#include "engine.h"
#include "event.h"

#include "world.h"
#include "kinematicsmodel.h"

#include "modelsystem.h"
#include "lightsystem.h"

#include "profiler.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"

#include "imgui/imgui_widget_flamegraph.h"

namespace Engine {
    namespace Render {

        ////////////////////// PUBLIC API ///////////////////////

        // Core
        PFN_R_SHOWWINDOW            ShowWindow             = NULL;
        PFN_R_SETUP                 Setup                  = NULL;
        PFN_R_INITIALIZE            Initialize             = NULL;
        PFN_R_DESTROY               Destroy                = NULL;
        PFN_R_SWAPBUFFERS           SwapBuffers            = NULL;
        PFN_R_GETINFO               GetInfo                = NULL;

        // R3D
        PFN_R3D_CREATEMATERIAL      R3D_CreateMaterial     = NULL;
        PFN_R3D_DESTROYMATERIAL     R3D_DestroyMaterial    = NULL;

        PFN_R3D_CREATESTATICMODEL   R3D_CreateStaticModel  = NULL;
        PFN_R3D_DESTROYSTATICMODEL  R3D_DestroyStaticModel = NULL;

        PFN_R3D_CREATERIGGEDMODEL   R3D_CreateRiggedModel  = NULL;
        PFN_R3D_DESTROYRIGGEDMODEL  R3D_DestroyRiggedModel = NULL;

        PFN_R3D_CREATEBONEBUFFER    R3D_CreateBoneBuffer   = NULL;
        PFN_R3D_DESTROYBONEBUFFER   R3D_DestroyBoneBuffer  = NULL;
        PFN_R3D_UPDATEBONEBUFFER    R3D_UpdateBoneBuffer   = NULL;

        PFN_R3D_PUSHMODEL           R3D_PushModel          = NULL;
        PFN_R3D_SETCAMERA           R3D_SetCamera          = NULL;

        PFN_R3D_STARTRENDERTASK     R3D_StartRenderTask    = NULL;

        /////////////////////////////////////////////////////////

        static LibraryHandle        handle                 = NULL;
        static Bool                 isRendererLoaded       = false;

        static ivec2                wndSize                = { 0, 0 };

        static ModelSystem*         modelSystem            = NULL;
        static LightSystem*         lightSystem            = NULL;

        static RenderSetupInfo      setupParams            = {};

        ////////////////// R3D_RENDER //////////////////

        static R3D_RenderTaskInfo        renderTaskInfo    = {};
        static R3D_GlobalLightDescrition glightdescription = {};

        static bool _EventHandler(SDL_Event* event) {
#if 0
            if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_F11) {
                ToggleFullscreen();
            }
#endif

            if (event->type == SDL_WINDOWEVENT) {
                switch (event->window.event) {
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {

                        GetWindowSize(&wndSize);

                        ImGuiIO& io = ImGui::GetIO();
                        io.DisplaySize.x = wndSize.x;
                        io.DisplaySize.y = wndSize.y;

                        //rgLogWarn(RG_LOG_RENDER, "Size changed: %dx%d", (Uint32)wndSize.x, (Uint32)wndSize.y);
                        PushEvent(0, RG_EVENT_RENDER_VIEWPORT_RESIZE, &wndSize, NULL);
                        break;
                    }
                    default: { break; }
                }
            }

            return true;
        }

        void LoadRenderer(String path) {

            RegisterEventHandler(_EventHandler);
            handle = Engine::DL_LoadLibrary(path);


            glightdescription.ambient = 0.4;
            glightdescription.intensity = 6;
            glightdescription.time = 1.7;
            glightdescription.color = {1, 0.8f, 0.7f};

            renderTaskInfo.globallight = &glightdescription;


            // Core
            ShowWindow  = (PFN_R_SHOWWINDOW)Engine::DL_GetProcAddress(handle, "R_ShowWindow");
            Setup       = (PFN_R_SETUP)Engine::DL_GetProcAddress(handle, "R_Setup");
            Initialize  = (PFN_R_INITIALIZE)Engine::DL_GetProcAddress(handle, "R_Initialize");
            Destroy     = (PFN_R_DESTROY)Engine::DL_GetProcAddress(handle, "R_Destroy");
            SwapBuffers = (PFN_R_SWAPBUFFERS)Engine::DL_GetProcAddress(handle, "R_SwapBuffers");
            GetInfo     = (PFN_R_GETINFO)Engine::DL_GetProcAddress(handle, "R_GetInfo");

            // R3D
            R3D_CreateMaterial     = (PFN_R3D_CREATEMATERIAL)Engine::DL_GetProcAddress(handle, "R3D_CreateMaterial");
            R3D_DestroyMaterial    = (PFN_R3D_DESTROYMATERIAL)Engine::DL_GetProcAddress(handle, "R3D_DestroyMaterial");

            R3D_CreateStaticModel  = (PFN_R3D_CREATESTATICMODEL)Engine::DL_GetProcAddress(handle, "R3D_CreateStaticModel");
            R3D_DestroyStaticModel = (PFN_R3D_DESTROYSTATICMODEL)Engine::DL_GetProcAddress(handle, "R3D_DestroyStaticModel");

            R3D_CreateRiggedModel  = (PFN_R3D_CREATERIGGEDMODEL)Engine::DL_GetProcAddress(handle, "R3D_CreateRiggedModel");
            R3D_DestroyRiggedModel = (PFN_R3D_DESTROYRIGGEDMODEL)Engine::DL_GetProcAddress(handle, "R3D_DestroyRiggedModel");

            R3D_CreateBoneBuffer   = (PFN_R3D_CREATEBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_CreateBoneBuffer");
            R3D_DestroyBoneBuffer  = (PFN_R3D_DESTROYBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_DestroyBoneBuffer");
            R3D_UpdateBoneBuffer   = (PFN_R3D_UPDATEBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_UpdateBoneBuffer");

            R3D_PushModel          = (PFN_R3D_PUSHMODEL)Engine::DL_GetProcAddress(handle, "R3D_PushModel");
            R3D_SetCamera          = (PFN_R3D_SETCAMERA)Engine::DL_GetProcAddress(handle, "R3D_SetCamera");

            R3D_StartRenderTask    = (PFN_R3D_STARTRENDERTASK)Engine::DL_GetProcAddress(handle, "R3D_StartRenderTask");

            modelSystem = RG_NEW_CLASS(GetDefaultAllocator(), ModelSystem)();
            lightSystem = RG_NEW_CLASS(GetDefaultAllocator(), LightSystem)();

            isRendererLoaded = true;

        }

        void UnloadRenderer() {

            RG_DELETE_CLASS(GetDefaultAllocator(), ModelSystem, modelSystem);
            RG_DELETE_CLASS(GetDefaultAllocator(), LightSystem, lightSystem);
            
            FreeEventHandler(_EventHandler);
            DL_UnloadLibrary(handle);

            ShowWindow  = NULL;
            Setup       = NULL;
            Initialize  = NULL;
            Destroy     = NULL;
            SwapBuffers = NULL;
            GetInfo     = NULL;

            R3D_CreateMaterial     = NULL;
            R3D_DestroyMaterial    = NULL;
            R3D_CreateStaticModel  = NULL;
            R3D_DestroyStaticModel = NULL;
            R3D_CreateRiggedModel  = NULL;
            R3D_DestroyRiggedModel = NULL;
            R3D_CreateBoneBuffer   = NULL;
            R3D_DestroyBoneBuffer  = NULL;
            R3D_UpdateBoneBuffer   = NULL;
            R3D_PushModel          = NULL;
            R3D_SetCamera          = NULL;
            R3D_StartRenderTask    = NULL;

            isRendererLoaded = false;
        }

        Bool IsRendererLoaded() {
            return isRendererLoaded;
        }

        LibraryHandle GetHandle() {
            return handle;
        }

        void InitSubSystem() {
            GetWindowSize(&wndSize);




#if 0
            rgLogInfo(RG_LOG_SYSTEM, "R3D Test");
            R3DCreateMaterialInfo minfo = {};
            R3D_Material* mat_ptr = R3D_CreateMaterial(&minfo);

            R3DCreateStaticModelInfo sminfo = {};
            R3D_StaticModel* sm_ptr = R3D_CreateStaticModel(&sminfo);

            rgLogInfo(RG_LOG_SYSTEM, "Mat: %p, model: %p", mat_ptr, sm_ptr);

            R3D_DestroyStaticModel(sm_ptr);
            R3D_DestroyMaterial(mat_ptr);

            rgLogInfo(RG_LOG_SYSTEM, "~ ~ ~ ~ ~ ~ ~");
#endif
        }

        void DestroySubSystem() {

        }

        void DrawRendererStats() {
            RenderInfo renderer_info = {};
            GetInfo(&renderer_info);

            ImGui::Begin("Renderer stats");

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

            ImGui::Text("Fps: %.2f", 1.0f / GetDeltaTime());

            ImGui::End();
        }

        static void ProfilerValueGetter(float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) {
            Profiler* prof = (Profiler*)data;

            String  section = GetProfile(idx);
            Float64 time = prof->GetTime(section);

            Float64 start = 0;
            for (Uint32 i = 0; i < idx; i++) {
                start += prof->GetTime(GetProfile(i));
            }
            if (startTimestamp) { *startTimestamp = (Float32)start * 1000; }
            if (endTimestamp)   { *endTimestamp = (Float32)(start + time) * 1000; }
            if (level)          { *level = 0; }
            if (caption)        { *caption = section; }
        }

        void DrawProfilerStats() {
            ImGui::Begin("Main profiler");

            Profiler* prof = GetProfiler();
            ImGuiWidgetFlameGraph::PlotFlame("Main thread", ProfilerValueGetter, prof, prof->GetSectionCount(), 0, "Main Thread", FLT_MAX, FLT_MAX, ImVec2(400, 0));

            ImGui::End();
        }

        static void RenderWorld(World* world) {
            R3D_PushModelInfo info = {};

            for (Uint32 i = 0; i < world->GetEntityCount(); i++) {
                Entity* ent = world->GetEntity(i);

                info.matrix = *ent->GetTransform()->GetMatrix();

                ModelComponent* mc = ent->GetComponent(Component_MODELCOMPONENT)->AsModelComponent();
                if (mc) {
                    info.handle_static = mc->GetHandle();
                    Render::R3D_PushModel(&info);
                }

                RiggedModelComponent* rmc = ent->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();
                if (rmc) {
                    info.handle_rigged     = rmc->GetHandle();
                    info.handle_bonebuffer = rmc->GetKinematicsModel()->GetBufferHandle();
                    Render::R3D_PushModel(&info);
                }
            }
        }

        void Update() {

            modelSystem->UpdateComponents();
            lightSystem->UpdateComponents();

            RenderWorld(Engine::GetWorld());

            R3D_StartRenderTask(&renderTaskInfo);

            Window_Update();
        }

        void SetGlobalLight(R3D_GlobalLightDescrition* desc) {
            glightdescription = *desc;
        }

        //void ToggleConsole() {
        //
        //}

        ModelSystem* GetModelSystem() {
            return modelSystem;
        }

        LightSystem* GetLightSystem() {
            return lightSystem;
        }

        R3D_BoneBuffer* CreateBoneBuffer(R3DCreateBoneBufferInfo* info) {
            if (isRendererLoaded) {
                return R3D_CreateBoneBuffer(info);
            } else {
               return NULL;
            }
        }

        void DestroyBoneBuffer(R3D_BoneBuffer* hbuff) {
            if (isRendererLoaded) {
                R3D_DestroyBoneBuffer(hbuff);
            }
        }

        RenderSetupInfo* GetSetupParams() {
            return &setupParams;
        }

        void SetRenderFlags(RenderFlags flags) {
            setupParams.flags = flags;
        }

	}
}