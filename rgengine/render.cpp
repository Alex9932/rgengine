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

#include "guiwnds.h"
#include "profiler.h"
#include "frustum.h"

#include "material.h"
#include "texture.h"

#include "ranimator.h"
#include "rimgui.h"
#include "rgbuffer.h"
#include "rgbufferdraw.h"
#include "rlighting.h"


#include <vector>

#define R_BUFFER_COUNT 2

namespace Engine {
    namespace Render {

		static STDAllocator*        renderalloc            = NULL;
        static LibraryHandle        handle                 = NULL;
        static Bool                 isRendererLoaded       = false;

        static RenderBackend        renderctx              = {};
        static RRenderDevice*       rdev                   = NULL;


        static RCommandBuffer*      cmdbuffer              = NULL;

        static Bool isWindowResized = false;

        

        static struct mat_transform {
            mat4 proj;
            mat4 view;
        } mat_camera;

        // Test
		static Uint32 frameIndex = 0;
        //static RResourceView* backbuffer[R_BUFFER_COUNT] = {};
        //static RFramebuffer* framebuffer3d[R_BUFFER_COUNT] = {};

		//static RImage* depthbuffer = NULL;
		//static RResourceView* depthbuffer_rv = NULL;


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

            // SDL3 migration
            if (event->type == SDL_EVENT_WINDOW_RESIZED) {
                //switch (event->window.type) {
                    //case SDL_WINDOWEVENT_SIZE_CHANGED: {

                        GetWindowSize(&wndSize);

                        ImGuiIO& io = ImGui::GetIO();
                        io.DisplaySize.x = wndSize.x;
                        io.DisplaySize.y = wndSize.y;

                        //rgLogWarn(RG_LOG_RENDER, "Size changed: %dx%d", (Uint32)wndSize.x, (Uint32)wndSize.y);
                        PushEvent(0, RG_EVENT_RENDER_VIEWPORT_RESIZE, &wndSize, NULL);
                        isWindowResized = true;
                        //break;
                    //}
                    //default: { break; }
                //}
            }

            return true;
        }

        void LoadRenderer(String path) {

			// TODO: create custom allocator for renderer
			renderalloc = GetDefaultAllocator();

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

            renderctx.Setup();

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

        void InitSubSystem(SDL_Window* hwnd) {
            GetWindowSize(&wndSize);

            RRenderSetupInfo setupinfo = {};
            setupinfo.flags = setupParams.flags;
            setupinfo.hwnd = hwnd;
            rdev = renderctx.CreateDevice(&setupinfo);

            InitializeMaterials();
            InitializeTextures();

            InitRenderAnimation();
            InitRImGui();
            InitRLighting();



            /////////////////////////////////////////////////////
            
            InitGBuffer(&wndSize);

#if 0
            
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
#endif

            //InitializeConsole();

        }

        void DestroySubSystem() {

            //DestroyConsole();

            //renderctx.R2D_DestroyBuffer(r2d_buffer);
            //renderctx.R2D_DestroyTexture(r2d_texture);
            //renderctx.R2D_DestroyTexture(r2d_texture_bg);

            DestroyRLighting();
            DestroyRImGui();
            DestroyRenderAnimation();
            DestroyGBuffer();

            DestroyMaterials();
            DestroyTextures();

            renderctx.DestroyDevice(rdev);

        }


        SDL_Window* ShowWindow(Uint32 w, Uint32 h) {
            return renderctx.ShowWindow(w, h);
        }

        void SwapBuffers() {
            RSwapBuffersInfo sbinfo = {};

            if (isWindowResized) {
                // Free swapchain resources
                //DestroyFramebuffers();

                sbinfo.flags |= RG_SWAPCHAIN_FLAG_RESIZE;
                GetWindowSize(&wndSize);
                sbinfo.newsize = wndSize;
            }

            renderctx.SwapBuffers(rdev, &sbinfo);

            frameIndex++;
            frameIndex = frameIndex % R_BUFFER_COUNT;

            DoLoadTextures();
            
            if (isWindowResized) {
                isWindowResized = false;

                // Recreate swapchain, framebuffers and renderpasses
                //CreateFramebuffers();
                ResizeGBuffer(&wndSize);
                frameIndex = 0;
            }
        }

        RRenderDevice* GetRenderDevice() {
            return rdev;
        }

        RenderBackend* GetRenderContext() {
            return &renderctx;
        }

        
#if 0
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
#endif

        void SetCamera(R3D_CameraInfo* info) {

            // Undate frustum
            mat4 camera_view;
            mat4_view(&camera_view, info->position, info->rotation);
            CreateFrustumInfo finfo = {};
            finfo.result = &frustum;
            finfo.proj   = &info->projection;
            finfo.view   = &camera_view;
            CreateFrustum(&finfo);

            mat4 view;
			mat4_view(&mat_camera.view, info->position, info->rotation);
            mat_camera.proj = info->projection;

            //renderctx.R3D_SetCamera(info);
        }

        void UpdateSystems() {
            particlesystem->UpdateComponents(NULL);
        }

        void Update() {
			World* world = Engine::GetWorld();

            UpdateFrametime(GetDeltaTime());
            // Render scene
            // TODO

#if 1
            UpdateImGui();
            DoAnimate();

            BeginGBufferPass(&mat_camera.proj, &mat_camera.view);

			// Draw entities
            for (Uint32 i = 0; i < world->GetEntityCount(); i++) {
				Entity* ent = world->GetEntity(i);
                ModelComponent* mc = ent->GetComponent(Component_MODELCOMPONENT)->AsModelComponent();
                RiggedModelComponent* rmc = ent->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();

                mat4* matrix = ent->GetTransform()->GetMatrix();

                if (mc) { DrawGBufferStatic(mc->GetHandle(), matrix); }
                if (rmc) { DrawGBufferStatic(&rmc->GetHandle()->s_model, matrix); }
            }

            // Draw static models
            for (Uint32 i = 0; i < world->GetStaticCount(); i++) {
                StaticObject* staticobj = world->GetStaticObject(i);
                R3D_StaticModel* staticmdl = staticobj->GetModelHandle();
				DrawGBufferStatic(staticmdl, staticobj->GetMatrix());
            }


            EndGBufferPass();

#endif
            DoRLighting();

            DrawImGui();


#if 0
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

#endif

            //Window_Update();
        }

        void SetGlobalLight(R3D_GlobalLightDescrition* desc) {
            glightdescription = *desc;
        }

        void GetInfo(RenderInfo* info) {
            renderctx.GetInfo(rdev, info);
        }

        ParticleSystem* GetParticleSystem() {
            return particlesystem;
        }


        R3D_StaticModel* CreateStaticModel(R3DStaticModelInfo* info) {
            if (!isRendererLoaded) { return NULL; }

            R3D_StaticModel* staticmdl = (R3D_StaticModel*)renderalloc->Allocate(sizeof(R3D_StaticModel));
			staticmdl->type = R_MODEL_STATIC;
            staticmdl->mCount = info->mCount;
			staticmdl->info = (R3D_MeshInfo*)renderalloc->Allocate(sizeof(R3D_MeshInfo) * info->mCount);

            for (Uint32 i = 0; i < info->mCount; i++) {
                staticmdl->info[i].indexCount  = info->mInfo[i].indexCount;
                staticmdl->info[i].indexOffset = info->mInfo[i].indexOffset;

				Uint32 matidx = info->mInfo[i].materialIdx;
                staticmdl->info[i].material = GetMaterial(&info->matInfo[matidx]);
            }

            RBufferCreateInfo vbinfo = {};
            vbinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
            vbinfo.usage  = RG_BUFFER_USAGE_DEFAULT;
            vbinfo.type   = RG_BUFFER_TYPE_VERTEX;
            vbinfo.stride = sizeof(R3D_Vertex);
            vbinfo.length = sizeof(R3D_Vertex) * info->vCount;
            vbinfo.initialData = info->vertices;
            staticmdl->vBuffer = renderctx.CreateBuffer(rdev, &vbinfo);

            RBufferCreateInfo ibinfo = {};
            ibinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
            ibinfo.usage  = RG_BUFFER_USAGE_DEFAULT;
            ibinfo.type   = RG_BUFFER_TYPE_INDEX;
            ibinfo.stride = info->iType;
            ibinfo.length = info->iCount * info->iType;
            ibinfo.initialData = info->indices;
            staticmdl->iBuffer = renderctx.CreateBuffer(rdev, &ibinfo);

            staticmdl->iCount = info->iCount;
            staticmdl->iType  = info->iType;

            return staticmdl;
        }

        void DestroyStaticModel(R3D_StaticModel* mdl) {
            if (!isRendererLoaded) { return; }

            for (Uint32 i = 0; i < mdl->mCount; i++) {
                FreeMaterial(mdl->info[i].material);
            }

			renderctx.DestroyBuffer(mdl->vBuffer);
			renderctx.DestroyBuffer(mdl->iBuffer);
			renderalloc->Deallocate(mdl->info);
			renderalloc->Deallocate(mdl);

            //renderctx.R3D_DestroyStaticModel(mdl);
        }

        R3D_RiggedModel* CreateRiggedModel(R3DRiggedModelInfo* info) {
            if (!isRendererLoaded) { return NULL; }
            
            R3D_RiggedModel* rigmdl = (R3D_RiggedModel*)renderalloc->Allocate(sizeof(R3D_RiggedModel));
            rigmdl->type   = R_MODEL_RIGGED;
			rigmdl->vCount = info->vCount;

            rigmdl->s_model.mCount = info->mCount;
            rigmdl->s_model.info = (R3D_MeshInfo*)renderalloc->Allocate(sizeof(R3D_MeshInfo) * info->mCount);

            for (Uint32 i = 0; i < info->mCount; i++) {
                rigmdl->s_model.info[i].indexCount = info->mInfo[i].indexCount;
                rigmdl->s_model.info[i].indexOffset = info->mInfo[i].indexOffset;

                Uint32 matidx = info->mInfo[i].materialIdx;
                rigmdl->s_model.info[i].material = GetMaterial(&info->matInfo[matidx]);
            }

			// Vertex buffer for skinning output and model rendering
            RBufferCreateInfo vbinfo = {};
            vbinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
            vbinfo.usage  = RG_BUFFER_USAGE_DEFAULT;

            // We MUST use the RG_BUFFER_TYPE_VERTEX flag
            // its working perfectly on Nvidia drivers without this flag LoL
			// TODO: Add another buffer with this flag for copy data and rendering (bind as vertex buffer)
            
            vbinfo.type   = RG_BUFFER_TYPE_VERTEX | RG_BUFFER_TYPE_SHADER_RES | RG_BUFFER_TYPE_UNORDERED | RG_BUFFER_TYPE_STRUCTURED;
            //vbinfo.type   = RG_BUFFER_TYPE_SHADER_RES | RG_BUFFER_TYPE_UNORDERED | RG_BUFFER_TYPE_STRUCTURED;
            vbinfo.stride = sizeof(R3D_Vertex);
            vbinfo.length = sizeof(R3D_Vertex) * info->vCount;
            //vbinfo.initialData = info->vertices; // Not needed (generated dynamicly by compute shader)
            rigmdl->s_model.vBuffer = renderctx.CreateBuffer(rdev, &vbinfo);

			// Index buffer
            RBufferCreateInfo ibinfo = {};
            ibinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
            ibinfo.usage  = RG_BUFFER_USAGE_DEFAULT;
            ibinfo.type   = RG_BUFFER_TYPE_INDEX;
            ibinfo.stride = info->iType;
            ibinfo.length = info->iCount * info->iType;
            ibinfo.initialData = info->indices;
            rigmdl->s_model.iBuffer = renderctx.CreateBuffer(rdev, &ibinfo);

            rigmdl->s_model.iCount = info->iCount;
            rigmdl->s_model.iType = info->iType;

			// Vertex buffer for skinning input
            vbinfo = {};
            vbinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
            vbinfo.usage  = RG_BUFFER_USAGE_DEFAULT;
            vbinfo.type   = RG_BUFFER_TYPE_SHADER_RES | RG_BUFFER_TYPE_STRUCTURED;
            vbinfo.stride = sizeof(R3D_Vertex);
            vbinfo.length = sizeof(R3D_Vertex) * info->vCount;
            vbinfo.initialData = info->vertices;
            rigmdl->i_vertex = renderctx.CreateBuffer(rdev, &vbinfo);

			// Vertex weights buffer
			RBufferCreateInfo wbuffinfo = {};
			wbuffinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
			wbuffinfo.usage  = RG_BUFFER_USAGE_DEFAULT;
			wbuffinfo.type   = RG_BUFFER_TYPE_SHADER_RES | RG_BUFFER_TYPE_STRUCTURED;
            wbuffinfo.stride = sizeof(R3D_Weight);
            wbuffinfo.length = sizeof(R3D_Weight) * info->vCount;
            wbuffinfo.initialData = info->weights;
            rigmdl->i_weight = renderctx.CreateBuffer(rdev, &wbuffinfo);

            RDescriptorSetBinding bindings[3] = {};
            bindings[0].binding = 0;
            bindings[0].stage   = RG_SHADER_TYPE_COMPUTE;
            bindings[0].type    = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[0].buffer  = rigmdl->i_vertex;
            bindings[1].binding = 1;
            bindings[1].stage   = RG_SHADER_TYPE_COMPUTE;
            bindings[1].type    = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[1].buffer  = rigmdl->i_weight;
            bindings[2].binding = 2;
            bindings[2].stage   = RG_SHADER_TYPE_COMPUTE;
            bindings[2].type    = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[2].buffer  = rigmdl->s_model.vBuffer;

            RDescriptorSetCreateInfo dsinfo = {};
            dsinfo.binding_count = 3;
            dsinfo.bindings = bindings;

            rigmdl->set = renderctx.CreateDescriptorSet(rdev, &dsinfo);

            return rigmdl;
        }

        void DestroyRiggedModel(R3D_RiggedModel* mdl) {
            if (!isRendererLoaded) { return; }

            for (Uint32 i = 0; i < mdl->s_model.mCount; i++) {
                FreeMaterial(mdl->s_model.info[i].material);
            }
            
            renderctx.DestroyDescriptorSet(mdl->set);
			renderctx.DestroyBuffer(mdl->i_vertex);
			renderctx.DestroyBuffer(mdl->i_weight);
			renderctx.DestroyBuffer(mdl->s_model.vBuffer);
			renderctx.DestroyBuffer(mdl->s_model.iBuffer);

			renderalloc->Deallocate(mdl->s_model.info);
			renderalloc->Deallocate(mdl);

        }

        R3D_BoneBuffer* CreateBoneBuffer(R3DCreateBufferInfo* info) {
            if (!isRendererLoaded) { return NULL; }
			R3D_BoneBuffer* bonebuf = (R3D_BoneBuffer*)renderalloc->Allocate(sizeof(R3D_BoneBuffer));

			RBufferCreateInfo buffinfo = {};
			buffinfo.access = RG_BUFFER_ACCESS_CPU_WRITE;
			buffinfo.usage  = RG_BUFFER_USAGE_DYNAMIC;
			buffinfo.type   = RG_BUFFER_TYPE_SHADER_RES | RG_BUFFER_TYPE_STRUCTURED;
            buffinfo.stride = sizeof(mat4);
            buffinfo.length = info->len;
            buffinfo.initialData = info->initialData;

            bonebuf->buffer = renderctx.CreateBuffer(rdev, &buffinfo);

            RDescriptorSetBinding binding = {};
            binding.binding = 0;
            binding.stage   = RG_SHADER_TYPE_COMPUTE;
            binding.type    = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.buffer  = bonebuf->buffer;

            RDescriptorSetCreateInfo dsinfo = {};
            dsinfo.binding_count = 1;
            dsinfo.bindings = &binding;

            bonebuf->set = renderctx.CreateDescriptorSet(rdev, &dsinfo);

            return bonebuf;
        }

        void DestroyBoneBuffer(R3D_BoneBuffer* hbuff) {
            if (!isRendererLoaded) { return; }

            renderctx.DestroyDescriptorSet(hbuff->set);
			renderctx.DestroyBuffer(hbuff->buffer);

			renderalloc->Deallocate(hbuff);
        }

        void UpdateBoneBuffer(R3DUpdateBufferInfo* info) {
            if (!isRendererLoaded) { return; }

			RUpdateBufferInfo ubinfo = {};
            ubinfo.data   = info->data;
            ubinfo.handle = info->handle_bone->buffer;
			ubinfo.length = info->length;
			ubinfo.offset = info->offset;
            renderctx.UpdateBuffer(&ubinfo);
            //renderctx.R3D_UpdateBoneBuffer(info);
        }

        R3D_AtlasHandle* CreateAtlas(String texture) {
            if (!isRendererLoaded) { return NULL; }
            //return renderctx.R3D_CreateAtlas(texture);
            return NULL;
        }

        void DestroyAtlas(R3D_AtlasHandle* atlas) {
            if (!isRendererLoaded) { return; }
            //renderctx.R3D_DestroyAtlas(atlas);
        }

        R3D_ParticleBuffer* CreateParticleBuffer(R3DCreateBufferInfo* info) {
            if (!isRendererLoaded) { return NULL; }
            //return renderctx.R3D_CreateParticleBuffer(info);
            return NULL;
        }

        void DestroyParticleBuffer(R3D_ParticleBuffer* hbuff) {
            if (!isRendererLoaded) { return; }
            //renderctx.R3D_DestroyParticleBuffer(hbuff);
        }

        void UpdateParticleBuffer(R3DUpdateBufferInfo* info) {
            if (!isRendererLoaded) { return; }
            //renderctx.R3D_UpdateParticleBuffer(info);
        }

        RenderSetupInfo* GetSetupParams() {
            return &setupParams;
        }

        void SetRenderFlags(Uint32 flags) {
            setupParams.flags = flags;
        }

    }
}