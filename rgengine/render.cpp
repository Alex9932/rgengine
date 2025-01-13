#define DLL_EXPORT
#include "render.h"
#include "window.h"
#include "engine.h"
#include "event.h"

#include "world.h"

#include "entity.h"
#include "staticobject.h"

#include "kinematicsmodel.h"

#include "modelsystem.h"
#include "lightsystem.h"
#include "particlesystem.h"

#include "console.h"

#include "profiler.h"
#include "frustum.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"

#include "imgui/imgui_widget_flamegraph.h"

namespace Engine {
    namespace Render {

        static Renderer             renderctx              = {};

        static LibraryHandle        handle                 = NULL;
        static Bool                 isRendererLoaded       = false;
        
        static Bool                 isEntityCullingEnabled = false;
        static Bool                 isStaticCullingEnabled = true;

        static ivec2                wndSize                = { 0, 0 };

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

            LoadRendererContext(&renderctx, handle);

            particlesystem = RG_NEW_CLASS(GetDefaultAllocator(), ParticleSystem)();

            isRendererLoaded = true;

            renderctx.Setup(&setupParams);

        }

        void UnloadRenderer() {

            RG_DELETE_CLASS(GetDefaultAllocator(), ParticleSystem, particlesystem);
            
            FreeEventHandler(_EventHandler);
            ClearRendererContext(&renderctx);
            DL_UnloadLibrary(handle);

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
            r2d_buffer = renderctx.R2D_CreateBuffer(&createbufferinfo);

            R2DCreateTextureInfo createtextureinfo = {};
            createtextureinfo.path = "platform/textures/loading.png";
            r2d_texture = renderctx.R2D_CreateTexture(&createtextureinfo);

            createtextureinfo.path = "platform/textures/loading_bg.png";
            r2d_texture_bg = renderctx.R2D_CreateTexture(&createtextureinfo);


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

            InitializeConsole();

        }

        void DestroySubSystem() {

            DestroyConsole();

            renderctx.R2D_DestroyBuffer(r2d_buffer);
            renderctx.R2D_DestroyTexture(r2d_texture);
            renderctx.R2D_DestroyTexture(r2d_texture_bg);

            renderctx.Destroy();
        }

        SDL_Window* ShowWindow(Uint32 w, Uint32 h) {
            return renderctx.ShowWindow(w, h);
        }

        void InitializeContext(SDL_Window* hwnd) {
            renderctx.Initialize(hwnd);
        }

        void SwapBuffers() {
            renderctx.SwapBuffers();
        }

        Renderer* GetRenderContext() {
            return &renderctx;
        }

        static Float32 ft_array[128] = {};
        static void UpdateFrametime(Float32 ft) {
            for (Sint32 i = 126; i >= 0; i--) {
                ft_array[i + 1] = ft_array[i];
            }
            ft_array[0] = ft;
        }

        static float FrametimeGetter(void* data, int idx) {
            Float32* ft_data = (Float32*)data;
            return ft_data[idx];
        }

        void DrawRendererStats() {
            RenderInfo renderer_info = {};
            renderctx.GetInfo(&renderer_info);

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
            
            ImGui::PlotLines("Frametime", FrametimeGetter, ft_array, 128);

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

        RG_FORCE_INLINE static void DrawEntity(R3D_PushModelInfo* info, Entity* ent) {
            ModelComponent* mc = ent->GetComponent(Component_MODELCOMPONENT)->AsModelComponent();
            RiggedModelComponent* rmc = ent->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();

            info->matrix = *ent->GetTransform()->GetMatrix();

            if (mc) {
                info->handle_static = mc->GetHandle();
                renderctx.R3D_PushModel(info);
            }
            
            if (rmc) {
                info->handle_rigged = rmc->GetHandle();
                info->handle_bonebuffer = rmc->GetKinematicsModel()->GetBufferHandle();
                renderctx.R3D_PushModel(info);
            }
        }

        static void ProcessEntities(R3D_PushModelInfo* info, World* world) {
            for (Uint32 i = 0; i < world->GetEntityCount(); i++) {
                Entity* ent = world->GetEntity(i);

                AABB aabb = *ent->GetAABB();
                aabb.Add(ent->GetTransform()->GetPosition());

                if (isEntityCullingEnabled) {
                    Bool inFrustum = AABBInFrustum(&frustum, &aabb);
                    if (!inFrustum) { continue; }
                }

                DrawEntity(info, ent);
            }
        }

        static void ProcessStatic(R3D_PushModelInfo* info, World* world) {
            for (Uint32 i = 0; i < world->GetStaticCount(); i++) {
                StaticObject* staticobj = world->GetStaticObject(i);
                
                mat4* mat = staticobj->GetMatrix();

                vec3 p = {};
                p.x = mat->m03;
                p.y = mat->m13;
                p.z = mat->m23;

                AABB aabb = *staticobj->GetAABB();
                aabb.Add(p);

                if (isStaticCullingEnabled) {
                    Bool inFrustum = AABBInFrustum(&frustum, &aabb);
                    if (!inFrustum) { continue; }
                }

                info->matrix        = *staticobj->GetMatrix();
                info->handle_static = staticobj->GetModelHandle();

                renderctx.R3D_PushModel(info);
            }
        }

        static void RenderWorld(World* world) {
            R3D_PushModelInfo info = {};

            UpdateFrametime(GetDeltaTime());

            // Draw static geometry
            ProcessStatic(&info, world);

            // Draw dynamic entities
            ProcessEntities(&info, world);

            // Push light sources
            // TODO: Add optimizations
            for (Uint32 i = 0; i < world->GetLightCount(); i++) {
                LightSource* src = world->GetLightSource(i);
                renderctx.R3D_PushLightSource(&src->source);
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

            renderctx.R3D_SetCamera(info);
        }

        void UpdateSystems() {
            particlesystem->UpdateComponents(NULL);
        }

        void Update() {

            RenderWorld(Engine::GetWorld());

            renderctx.R3D_StartRenderTask(&renderTaskInfo);


            ///////////////////////////
            // R2D


            RenderInfo renderer_info = {};
            renderctx.GetInfo(&renderer_info);
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
            

            renderctx.R2D_Begin();



            UpdateConsole();

            R2DBindInfo bindinfo = {};

            bindinfo.texture = r2d_texture_bg;
            bindinfo.buffer  = r2d_buffer;
            bindinfo.color   = {1, 1, 1, alpha };

            R2DDrawInfo drawinfo = {};
            drawinfo.offset = 0;
            drawinfo.count  = 6;

            renderctx.R2D_ResetStack();


            Float32 s1 = 2.0f;
            mat4 m13 = {
                s1,  0,  0, 0,
                 0, s1,  0, 0,
                 0,  0, s1, 0,
                 0,  0,  0, 1
            };

            renderctx.R2D_PushMatrix(&m13);

            renderctx.R2D_Bind(&bindinfo);
            renderctx.R2D_Draw(&drawinfo);

            renderctx.R2D_PopMatrix();

            
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
            

            renderctx.R2D_PushMatrix(&m0);
            renderctx.R2D_PushMatrix(&m2);
            renderctx.R2D_PushMatrix(&m3);
            renderctx.R2D_PushMatrix(&m1);


            bindinfo.texture = r2d_texture;
            bindinfo.buffer  = r2d_buffer;

            renderctx.R2D_Bind(&bindinfo);
            renderctx.R2D_Draw(&drawinfo);

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

        void GetInfo(RenderInfo* info) {
            renderctx.GetInfo(info);
        }

        ParticleSystem* GetParticleSystem() {
            return particlesystem;
        }


        R3D_StaticModel* CreateStaticModel(R3DStaticModelInfo* info) {
            if (!isRendererLoaded) { return NULL; }
            return renderctx.R3D_CreateStaticModel(info);
        }

        void DestroyStaticModel(R3D_StaticModel* mdl) {
            if (!isRendererLoaded) { return; }
            renderctx.R3D_DestroyStaticModel(mdl);
        }

        R3D_RiggedModel* CreateRiggedModel(R3DRiggedModelInfo* info) {
            if (!isRendererLoaded) { return NULL; }
            return renderctx.R3D_CreateRiggedModel(info);
        }

        void DestroyRiggedModel(R3D_RiggedModel* mdl) {
            if (!isRendererLoaded) { return; }
            renderctx.R3D_DestroyRiggedModel(mdl);
        }

        R3D_BoneBuffer* CreateBoneBuffer(R3DCreateBufferInfo* info) {
            if (!isRendererLoaded) { return NULL; }
            return renderctx.R3D_CreateBoneBuffer(info);
        }

        void DestroyBoneBuffer(R3D_BoneBuffer* hbuff) {
            if (!isRendererLoaded) { return; }
            renderctx.R3D_DestroyBoneBuffer(hbuff);
        }

        void UpdateBoneBuffer(R3DUpdateBufferInfo* info) {
            if (!isRendererLoaded) { return; }
            renderctx.R3D_UpdateBoneBuffer(info);
        }

        R3D_AtlasHandle* CreateAtlas(String texture) {
            if (!isRendererLoaded) { return NULL; }
            return renderctx.R3D_CreateAtlas(texture);
        }

        void DestroyAtlas(R3D_AtlasHandle* atlas) {
            if (!isRendererLoaded) { return; }
            renderctx.R3D_DestroyAtlas(atlas);
        }

        R3D_ParticleBuffer* CreateParticleBuffer(R3DCreateBufferInfo* info) {
            if (!isRendererLoaded) { return NULL; }
            return renderctx.R3D_CreateParticleBuffer(info);
        }

        void DestroyParticleBuffer(R3D_ParticleBuffer* hbuff) {
            if (!isRendererLoaded) { return; }
            renderctx.R3D_DestroyParticleBuffer(hbuff);
        }

        void UpdateParticleBuffer(R3DUpdateBufferInfo* info) {
            if (!isRendererLoaded) { return; }
            renderctx.R3D_UpdateParticleBuffer(info);
        }

        RenderSetupInfo* GetSetupParams() {
            return &setupParams;
        }

        void SetRenderFlags(Uint32 flags) {
            setupParams.flags = flags;
        }

    }
}