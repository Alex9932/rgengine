#define DLL_EXPORT
#include "render.h"
#include "window.h"
#include "engine.h"
#include "event.h"

#include "world.h"
#include "kinematicsmodel.h"

#include "modelsystem.h"
#include "lightsystem.h"
#include "particlesystem.h"

#include "profiler.h"
#include "frustum.h"

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

        // R2D
        PFN_R2D_CREATEBUFFER        R2D_CreateBuffer       = NULL;
        PFN_R2D_DESTROYBUFFER       R2D_DestroyBuffer      = NULL;
        PFN_R2D_BUFFERDATA          R2D_BufferData         = NULL;
        PFN_R2D_CREATETEXTURE       R2D_CreateTexture      = NULL;
        PFN_R2D_DESTROYTEXTURE      R2D_DestroyTexture     = NULL;
        PFN_R2D_TEXTUREDATA         R2D_TextureData        = NULL;
        PFN_R2D_PUSHMATRIX          R2D_PushMatrix         = NULL;
        PFN_R2D_POPMATRIX           R2D_PopMatrix          = NULL;
        PFN_R2D_RESETSTACK          R2D_ResetStack         = NULL;
        PFN_R2D_BEGIN               R2D_Begin              = NULL;
        PFN_R2D_BIND                R2D_Bind               = NULL;
        PFN_R2D_DRAW                R2D_Draw               = NULL;

        // R3D
        PFN_R3D_CREATEMATERIAL        R3D_CreateMaterial        = NULL;
        PFN_R3D_DESTROYMATERIAL       R3D_DestroyMaterial       = NULL;

        PFN_R3D_CREATESTATICMODEL     R3D_CreateStaticModel     = NULL;
        PFN_R3D_DESTROYSTATICMODEL    R3D_DestroyStaticModel    = NULL;

        PFN_R3D_CREATERIGGEDMODEL     R3D_CreateRiggedModel     = NULL;
        PFN_R3D_DESTROYRIGGEDMODEL    R3D_DestroyRiggedModel    = NULL;

        PFN_R3D_CREATEBONEBUFFER      R3D_CreateBoneBuffer      = NULL;
        PFN_R3D_DESTROYBONEBUFFER     R3D_DestroyBoneBuffer     = NULL;
        PFN_R3D_UPDATEBONEBUFFER      R3D_UpdateBoneBuffer      = NULL;

        PFN_R3D_CREATEATLAS           R3D_CreateAtlas           = NULL;
        PFN_R3D_DESTROYATLAS          R3D_DestroyAtlas          = NULL;

        PFN_R3D_CREATEPARTICLEBUFFER  R3D_CreateParticleBuffer  = NULL;
        PFN_R3D_DESTROYPARTICLEBUFFER R3D_DestroyParticleBuffer = NULL;
        PFN_R3D_UPDATEPARTICLEBUFFER  R3D_UpdateParticleBuffer  = NULL;

        PFN_R3D_PUSHMODEL             R3D_PushModel             = NULL;
        PFN_R3D_SETCAMERA             R3D_SetCamera             = NULL;

        PFN_R3D_STARTRENDERTASK       R3D_StartRenderTask       = NULL;

        /////////////////////////////////////////////////////////

        static LibraryHandle        handle                 = NULL;
        static Bool                 isRendererLoaded       = false;

        static ivec2                wndSize                = { 0, 0 };

        static ModelSystem*         modelSystem            = NULL;
        static LightSystem*         lightSystem            = NULL;
        static ParticleSystem*      particlesystem         = NULL;

        static RenderSetupInfo      setupParams            = {};

        static Frustum              frustum                = {};

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

            // R2D
            R2D_CreateBuffer   = (PFN_R2D_CREATEBUFFER)Engine::DL_GetProcAddress(handle, "R2D_CreateBuffer");
            R2D_DestroyBuffer  = (PFN_R2D_DESTROYBUFFER)Engine::DL_GetProcAddress(handle, "R2D_DestroyBuffer");
            R2D_BufferData     = (PFN_R2D_BUFFERDATA)Engine::DL_GetProcAddress(handle, "R2D_BufferData");
            R2D_CreateTexture  = (PFN_R2D_CREATETEXTURE)Engine::DL_GetProcAddress(handle, "R2D_CreateTexture");
            R2D_DestroyTexture = (PFN_R2D_DESTROYTEXTURE)Engine::DL_GetProcAddress(handle, "R2D_DestroyTexture");
            R2D_TextureData    = (PFN_R2D_TEXTUREDATA)Engine::DL_GetProcAddress(handle, "R2D_TextureData");
            R2D_PushMatrix     = (PFN_R2D_PUSHMATRIX)Engine::DL_GetProcAddress(handle, "R2D_PushMatrix");
            R2D_PopMatrix      = (PFN_R2D_POPMATRIX)Engine::DL_GetProcAddress(handle, "R2D_PopMatrix");
            R2D_ResetStack     = (PFN_R2D_RESETSTACK)Engine::DL_GetProcAddress(handle, "R2D_ResetStack");
            R2D_Begin          = (PFN_R2D_BEGIN)Engine::DL_GetProcAddress(handle, "R2D_Begin");
            R2D_Bind           = (PFN_R2D_BIND)Engine::DL_GetProcAddress(handle, "R2D_Bind");
            R2D_Draw           = (PFN_R2D_DRAW)Engine::DL_GetProcAddress(handle, "R2D_Draw");

            // R3D
            R3D_CreateMaterial        = (PFN_R3D_CREATEMATERIAL)Engine::DL_GetProcAddress(handle, "R3D_CreateMaterial");
            R3D_DestroyMaterial       = (PFN_R3D_DESTROYMATERIAL)Engine::DL_GetProcAddress(handle, "R3D_DestroyMaterial");

            R3D_CreateStaticModel     = (PFN_R3D_CREATESTATICMODEL)Engine::DL_GetProcAddress(handle, "R3D_CreateStaticModel");
            R3D_DestroyStaticModel    = (PFN_R3D_DESTROYSTATICMODEL)Engine::DL_GetProcAddress(handle, "R3D_DestroyStaticModel");

            R3D_CreateRiggedModel     = (PFN_R3D_CREATERIGGEDMODEL)Engine::DL_GetProcAddress(handle, "R3D_CreateRiggedModel");
            R3D_DestroyRiggedModel    = (PFN_R3D_DESTROYRIGGEDMODEL)Engine::DL_GetProcAddress(handle, "R3D_DestroyRiggedModel");

            R3D_CreateBoneBuffer      = (PFN_R3D_CREATEBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_CreateBoneBuffer");
            R3D_DestroyBoneBuffer     = (PFN_R3D_DESTROYBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_DestroyBoneBuffer");
            R3D_UpdateBoneBuffer      = (PFN_R3D_UPDATEBONEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_UpdateBoneBuffer");

            R3D_CreateAtlas           = (PFN_R3D_CREATEATLAS)Engine::DL_GetProcAddress(handle, "R3D_CreateAtlas");
            R3D_DestroyAtlas          = (PFN_R3D_DESTROYATLAS)Engine::DL_GetProcAddress(handle, "R3D_DestroyAtlas");

            R3D_CreateParticleBuffer  = (PFN_R3D_CREATEPARTICLEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_CreateParticleBuffer");
            R3D_DestroyParticleBuffer = (PFN_R3D_DESTROYPARTICLEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_DestroyParticleBuffer");
            R3D_UpdateParticleBuffer  = (PFN_R3D_UPDATEPARTICLEBUFFER)Engine::DL_GetProcAddress(handle, "R3D_UpdateParticleBuffer");

            R3D_PushModel             = (PFN_R3D_PUSHMODEL)Engine::DL_GetProcAddress(handle, "R3D_PushModel");
            R3D_SetCamera             = (PFN_R3D_SETCAMERA)Engine::DL_GetProcAddress(handle, "R3D_SetCamera");

            R3D_StartRenderTask       = (PFN_R3D_STARTRENDERTASK)Engine::DL_GetProcAddress(handle, "R3D_StartRenderTask");

            modelSystem = RG_NEW_CLASS(GetDefaultAllocator(), ModelSystem)();
            lightSystem = RG_NEW_CLASS(GetDefaultAllocator(), LightSystem)();
            particlesystem = RG_NEW_CLASS(GetDefaultAllocator(), ParticleSystem)();

            isRendererLoaded = true;

        }

        void UnloadRenderer() {

            RG_DELETE_CLASS(GetDefaultAllocator(), ModelSystem, modelSystem);
            RG_DELETE_CLASS(GetDefaultAllocator(), LightSystem, lightSystem);
            RG_DELETE_CLASS(GetDefaultAllocator(), ParticleSystem, particlesystem);
            
            FreeEventHandler(_EventHandler);
            DL_UnloadLibrary(handle);

            ShowWindow  = NULL;
            Setup       = NULL;
            Initialize  = NULL;
            Destroy     = NULL;
            SwapBuffers = NULL;
            GetInfo     = NULL;

            R2D_CreateBuffer   = NULL;
            R2D_DestroyBuffer  = NULL;
            R2D_BufferData     = NULL;
            R2D_CreateTexture  = NULL;
            R2D_DestroyTexture = NULL;
            R2D_TextureData    = NULL;
            R2D_PushMatrix     = NULL;
            R2D_PopMatrix      = NULL;
            R2D_ResetStack     = NULL;
            R2D_Begin          = NULL;
            R2D_Bind           = NULL;
            R2D_Draw           = NULL;

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

        static R2D_Buffer* r2d_buffer = NULL;
        static R2D_Texture* r2d_texture = NULL;
        static R2D_Texture* r2d_texture_bg = NULL;

        void InitSubSystem() {
            GetWindowSize(&wndSize);


            R2D_Vertex r2d_vertices[] = {
                /*
                { -0.5f, -0.5f, 0.0f, 1.0f, 1, 0, 0, 1 },
                {  0.0f,  0.5f, 0.5f, 0.0f, 0, 1, 0, 1 },
                {  0.5f, -0.5f, 1.0f, 1.0f, 0, 0, 1, 1 }
                */
                { -0.5f, -0.5f, 0.0f, 1.0f, 1, 1, 1, 1 },
                { -0.5f,  0.5f, 0.0f, 0.0f, 1, 1, 1, 1 },
                {  0.5f,  0.5f, 1.0f, 0.0f, 1, 1, 1, 1 },
                {  0.5f,  0.5f, 1.0f, 0.0f, 1, 1, 1, 1 },
                {  0.5f, -0.5f, 1.0f, 1.0f, 1, 1, 1, 1 },
                { -0.5f, -0.5f, 0.0f, 1.0f, 1, 1, 1, 1 }
             };

            R2DCreateBufferInfo createbufferinfo = {};
            createbufferinfo.length = 6;
            createbufferinfo.initial_data = r2d_vertices;
            r2d_buffer = R2D_CreateBuffer(&createbufferinfo);

            R2DCreateTextureInfo createtextureinfo = {};
            createtextureinfo.path = "platform/textures/loading.png";
            r2d_texture = R2D_CreateTexture(&createtextureinfo);

            createtextureinfo.path = "platform/textures/loading_bg.png";
            r2d_texture_bg = R2D_CreateTexture(&createtextureinfo);


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

            R2D_DestroyBuffer(r2d_buffer);
            R2D_DestroyTexture(r2d_texture);
            R2D_DestroyTexture(r2d_texture_bg);

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

            ImGui::Text("Task: time ms");

            Uint32 sections = prof->GetSectionCount();
            for (Uint32 i = 0; i < sections; i++) {
                String  secsrc = GetProfile(i);
                Float64 time   = prof->GetTime(secsrc) * 1000;

                ImGui::Text("%s: %.3lfms", secsrc, time);
            }

            ImGuiWidgetFlameGraph::PlotFlame("Main thread", ProfilerValueGetter, prof, sections, 0, "Main Thread", FLT_MAX, FLT_MAX, ImVec2(400, 0));

            ImGui::End();
        }

        static void RenderWorld(World* world) {

            

            R3D_PushModelInfo info = {};

            for (Uint32 i = 0; i < world->GetEntityCount(); i++) {
                Entity* ent = world->GetEntity(i);

                AABB aabb = *ent->GetAABB();
                aabb.Add(ent->GetTransform()->GetPosition());

                Bool inFrustum = AABBInFrustum(&frustum, &aabb);
                if (!inFrustum) { continue; }

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

        void SetCamera(R3D_CameraInfo* info) {

            // Undate frustum
            mat4 camera_view;
            mat4_view(&camera_view, info->position, info->rotation);
            CreateFrustumInfo finfo = {};
            finfo.result = &frustum;
            finfo.proj   = &info->projection;
            finfo.view   = &camera_view;
            CreateFrustum(&finfo);

            R3D_SetCamera(info);
        }

        void UpdateSystems() {
            modelSystem->UpdateComponents();
            lightSystem->UpdateComponents();
            // TODO
            particlesystem->UpdateComponents(NULL);
        }

        void Update() {

            RenderWorld(Engine::GetWorld());

            R3D_StartRenderTask(&renderTaskInfo);


            ///////////////////////////
            // R2D


            RenderInfo renderer_info = {};
            GetInfo(&renderer_info);
            Float32 f = 1;
            static Float32 alpha = 1;
            if (renderer_info.textures_inQueue != 0) {
                f = 1.0f - ((Float32)renderer_info.textures_left / (Float32)renderer_info.textures_inQueue);
            }

            if (f >= 1) {
                alpha -= 0.7f * GetDeltaTime();
                if (alpha < 0) { alpha = 0; }
            } else {
                alpha = 1;
            }
            

            R2D_Begin();

            R2DBindInfo bindinfo = {};

            bindinfo.texture = r2d_texture_bg;
            bindinfo.buffer  = r2d_buffer;
            bindinfo.color   = {1, 1, 1, alpha };

            R2DDrawInfo drawinfo = {};
            drawinfo.offset = 0;
            drawinfo.count  = 6;

            R2D_ResetStack();


            Float32 s1 = 2.0f;
            mat4 m13 = {
                s1,  0,  0, 0,
                 0, s1,  0, 0,
                 0,  0, s1, 0,
                 0,  0,  0, 1
            };

            R2D_PushMatrix(&m13);

            R2D_Bind(&bindinfo);
            R2D_Draw(&drawinfo);

            R2D_PopMatrix();

            
            Float32 aspect = 16.0f / 9.0f;
            mat4 m0;
            mat4_ortho(&m0, -aspect, aspect, -1, 1, -1, 1);

            mat4 m1;
            mat4_rotatez(&m1, Engine::GetUptime() * 10);

            mat4 m2;
            mat4_translate(&m2, {1.6f, -0.8f, 0.0f});

            Float32 s = 0.18f;
            mat4 m3 = {
                s, 0, 0, 0,
                0, s, 0, 0,
                0, 0, s, 0,
                0, 0, 0, 1
            };
            

            R2D_PushMatrix(&m0);
            R2D_PushMatrix(&m2);
            R2D_PushMatrix(&m3);
            R2D_PushMatrix(&m1);


            bindinfo.texture = r2d_texture;
            bindinfo.buffer  = r2d_buffer;

            R2D_Bind(&bindinfo);
            R2D_Draw(&drawinfo);

            /*
            R2D_PopMatrix();
            R2D_PopMatrix();
            R2D_PopMatrix();
            R2D_Draw(&drawinfo);
            */

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

        ParticleSystem* GetParticleSystem() {
            return particlesystem;
        }

        R3D_BoneBuffer* CreateBoneBuffer(R3DCreateBufferInfo* info) {
            if (!isRendererLoaded) { return NULL; }
            return R3D_CreateBoneBuffer(info);
        }

        void DestroyBoneBuffer(R3D_BoneBuffer* hbuff) {
            if (!isRendererLoaded) { return; }
            R3D_DestroyBoneBuffer(hbuff);
        }

        void UpdateBoneBuffer(R3DUpdateBufferInfo* info) {
            if (!isRendererLoaded) { return; }
            R3D_UpdateBoneBuffer(info);
        }

        R3D_AtlasHandle* CreateAtlas(String texture) {
            if (!isRendererLoaded) { return NULL; }
            return R3D_CreateAtlas(texture);
        }

        void DestroyAtlas(R3D_AtlasHandle* atlas) {
            if (!isRendererLoaded) { return; }
            R3D_DestroyAtlas(atlas);
        }

        R3D_ParticleBuffer* CreateParticleBuffer(R3DCreateBufferInfo* info) {
            if (!isRendererLoaded) { return NULL; }
            return R3D_CreateParticleBuffer(info);
        }

        void DestroyParticleBuffer(R3D_ParticleBuffer* hbuff) {
            if (!isRendererLoaded) { return; }
            R3D_DestroyParticleBuffer(hbuff);
        }

        void UpdateParticleBuffer(R3DUpdateBufferInfo* info) {
            if (!isRendererLoaded) { return; }
            R3D_UpdateParticleBuffer(info);
        }

        RenderSetupInfo* GetSetupParams() {
            return &setupParams;
        }

        void SetRenderFlags(RenderFlags flags) {
            setupParams.flags = flags;
        }

    }
}