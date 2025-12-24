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

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"

#include <vector>

#define R_BUFFER_COUNT 2

enum ModelType {
    R_MODEL_STATIC = 0,
    R_MODEL_RIGGED = 1
};

typedef struct R3D_StaticModel {
    ModelType     type;
    Uint32        mCount;
    R3D_MeshInfo* info;
    RBuffer*      vBuffer;
    RBuffer*      iBuffer;
    Uint32        iCount;
    IndexType     iType;
} R3D_StaticModel;

typedef struct R3D_RiggedModel {
    ModelType       type;
    Uint32          vCount;
    // Input data
    RBuffer*        i_vertex;  // Input vertex data
    RBuffer*        i_weight;  // Input weight data
    RResourceView*  i_srv_vtx; // Shader resource view for vertex input data
    RResourceView*  i_srv_wht; // Shader resource view for weight input data
    // Output data
    R3D_StaticModel s_model;   // Static model / output vertex data
    RResourceView*  s_uav;     // Unordered access view for vertex output data
} R3D_RiggedModel;

typedef struct R3D_BoneBuffer {
    RBuffer*       buffer;
    RResourceView* rv;
} R3D_BoneBuffer;

namespace Engine {
    namespace Render {

		static STDAllocator*        renderalloc            = NULL;
        static LibraryHandle        handle                 = NULL;
        static Bool                 isRendererLoaded       = false;

        static RenderBackend        renderctx              = {};
        static RRenderDevice*       rdev                   = NULL;


        static RCommandBuffer*      cmdbuffer              = NULL;

        static Bool isWindowResized = false;

        static std::vector<RenderImGuiCallback> imguicallbacks;
        

        static struct mat_transform {
            mat4 viewproj;
            mat4 model;
        } mat_camera;

        // Test
		static Uint32 frameIndex = 0;
        static RResourceView* backbuffer[R_BUFFER_COUNT] = {};
        static RFramebuffer* framebuffer3d[R_BUFFER_COUNT] = {};
        static RRenderpass* renderpass3d = NULL;

		static RPipeline* pipeline3d = NULL;
		static RShader* shader_vs = NULL;
		static RShader* shader_ps = NULL;
		static RSampler* sampler_linear = NULL;
		static RImage* depthbuffer = NULL;
		static RResourceView* depthbuffer_rv = NULL;

        static RShader*   skinning_shader   = NULL;
        static RPipeline* skinning_pipeline = NULL;


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

        static void CreateFramebuffers() {

			RRect wndrect = {};
            wndrect.x = 0;
            wndrect.y = 0;
			wndrect.width  = wndSize.x;
			wndrect.height = wndSize.y;


            // Create 3d renderpass
            RImageCreateInfo dbinfo = {};
            dbinfo.format = RG_FORMAT_D32;
            dbinfo.width  = wndSize.x;
            dbinfo.height = wndSize.y;
            depthbuffer = renderctx.CreateImage(rdev, &dbinfo);

            RResourceViewCreateInfo drvinfo = {};
            drvinfo.type = RG_RESOURCEVIEW_TYPE_DSV;
            drvinfo.dst_image = depthbuffer;
            depthbuffer_rv = renderctx.CreateResourceView(rdev, &drvinfo);

            RRenderpassCreateInfo rp3dinfo = {};
            rp3dinfo.rt_count  = 1;
            rp3dinfo.use_depth = true;
            rp3dinfo.viewport  = wndrect;
            rp3dinfo.rt_formats[0] = RG_FORMAT_R8G8B8A8_UNORM;
            renderpass3d = renderctx.CreateRenderpass(rdev, &rp3dinfo);

            for (Uint32 i = 0; i < R_BUFFER_COUNT; i++) {
                
                // Get swapchain backbuffer and renderpass
                RResourceViewCreateInfo backbufferinfo = {};
                backbufferinfo.type = RG_RESOURCEVIEW_TYPE_BBV;
                backbufferinfo.buffer_type = RG_RESOURCEVIEW_IMAGE;
                backbufferinfo.var = i; // Use first buffer only
                backbuffer[i] = renderctx.CreateResourceView(rdev, &backbufferinfo);

                RFramebufferCreateInfo fbinfo = {};
			    fbinfo.width      = wndSize.x;
			    fbinfo.height     = wndSize.y;
			    fbinfo.rt_count   = 1;
			    fbinfo.rts[0]     = backbuffer[i];
			    fbinfo.dsv        = depthbuffer_rv;
			    fbinfo.renderpass = renderpass3d;
                framebuffer3d[i] = renderctx.CreateFramebuffer(rdev, &fbinfo);
            }

            RPipelineLayoutDescription layout = {};
            layout.binding_count = 4;
            layout.bindings[0].binding = 0;
            layout.bindings[0].stage   = RG_SHADER_TYPE_PIXEL;
            layout.bindings[0].type    = RG_DESCRIPTOR_TYPE_IMAGE;
            layout.bindings[1].binding = 1;
            layout.bindings[1].stage   = RG_SHADER_TYPE_PIXEL;
            layout.bindings[1].type    = RG_DESCRIPTOR_TYPE_IMAGE;
            layout.bindings[2].binding = 2;
            layout.bindings[2].stage   = RG_SHADER_TYPE_PIXEL;
            layout.bindings[2].type    = RG_DESCRIPTOR_TYPE_IMAGE;
            layout.bindings[3].binding = 3;
            layout.bindings[3].stage   = RG_SHADER_TYPE_PIXEL;
            layout.bindings[3].type    = RG_DESCRIPTOR_TYPE_SAMPLER;

			RPipelineInputDescription layoutdescriptions[4] = {};
            layoutdescriptions[0].format = RG_FORMAT_R32G32B32_FLOAT;
            layoutdescriptions[0].inputSlot = 0;
            layoutdescriptions[0].name = "POSITION";
            layoutdescriptions[1].format = RG_FORMAT_R32G32B32_FLOAT;
            layoutdescriptions[1].inputSlot = 0;
            layoutdescriptions[1].name = "NORMAL";
            layoutdescriptions[2].format = RG_FORMAT_R32G32B32_FLOAT;
            layoutdescriptions[2].inputSlot = 0;
            layoutdescriptions[2].name = "TANGENT";
            layoutdescriptions[3].format = RG_FORMAT_R32G32_FLOAT;
            layoutdescriptions[3].inputSlot = 0;
            layoutdescriptions[3].name = "VPOS";
            RPipelineCreateInfo plinfo = {};
            plinfo.type          = RG_PIPELINE_TYPE_GRAPHICS;
            plinfo.vertex_shader = shader_vs;
            plinfo.pixel_shader  = shader_ps;
			plinfo.inputCount    = 4;
            plinfo.descriptions  = layoutdescriptions;
            plinfo.renderpass    = renderpass3d;
            plinfo.layout        = &layout;
            plinfo.cullmode      = RG_RENDERPASS_CULLMODE_BACK;
            plinfo.fillmode      = RG_RENDERPASS_FILLMODE_SOLID;

            pipeline3d = renderctx.CreatePipeline(rdev, &plinfo);
            

            RPipelineLayoutDescription cslayout = {};
            cslayout.binding_count = 4;
            // Matrices
            cslayout.bindings[0].binding = 0;
            cslayout.bindings[0].stage   = RG_SHADER_TYPE_COMPUTE;
            cslayout.bindings[0].type    = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            // Input vertices
            cslayout.bindings[1].binding = 1;
            cslayout.bindings[1].stage   = RG_SHADER_TYPE_COMPUTE;
            cslayout.bindings[1].type    = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            // Input weights
            cslayout.bindings[2].binding = 2;
            cslayout.bindings[2].stage   = RG_SHADER_TYPE_COMPUTE;
            cslayout.bindings[2].type    = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            // Output vertices
            cslayout.bindings[3].binding = 3;
            cslayout.bindings[3].stage   = RG_SHADER_TYPE_COMPUTE;
            cslayout.bindings[3].type    = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            RPipelineCreateInfo cplinfo = {};
            cplinfo.type = RG_PIPELINE_TYPE_COMPUTE;
            cplinfo.compute_shader = skinning_shader;
            cplinfo.inputCount     = 0;
            cplinfo.descriptions   = NULL;
            cplinfo.layout         = &cslayout;
            skinning_pipeline = renderctx.CreatePipeline(rdev, &cplinfo);

        }

        static void DestroyFramebuffers() {

            renderctx.DestroyPipeline(skinning_pipeline);

            renderctx.DestroyPipeline(pipeline3d);

            for (Uint32 i = 0; i < R_BUFFER_COUNT; i++) {
                renderctx.DestroyFramebuffer(framebuffer3d[i]);
                renderctx.DestroyResourceView(backbuffer[i]);
            }
            renderctx.DestroyRenderpass(renderpass3d);

            renderctx.DestroyImage(depthbuffer);
            renderctx.DestroyResourceView(depthbuffer_rv);

        }

        void InitSubSystem(SDL_Window* hwnd) {
            GetWindowSize(&wndSize);

            RRenderSetupInfo setupinfo = {};
            setupinfo.flags = setupParams.flags;
            setupinfo.hwnd = hwnd;
            rdev = renderctx.CreateDevice(&setupinfo);

            InitializeMaterials();
            InitializeTextures();


            RCommandBufferCreateInfo cmdbuffinfo = {};
            cmdbuffinfo.maxcmds = 128;
			cmdbuffer = renderctx.CreateCommandBuffer(rdev, &cmdbuffinfo);

            renderctx.ImGui_Init(rdev);

            RShaderCreateInfo csinfo = {};
            //csinfo.isCompiled = false;
            csinfo.isCompiled = true;
            csinfo.name = "skinning.cs";
            csinfo.type = RG_SHADER_TYPE_COMPUTE;
            skinning_shader = renderctx.CreateShader(rdev, &csinfo);


            /////////////////////////////////////////////////////
            RShaderCreateInfo vsinfo = {};
            //vsinfo.isCompiled = false;
            vsinfo.isCompiled = true;
            vsinfo.name = "fwd_test.vs";
            vsinfo.type = RG_SHADER_TYPE_VERTEX;
            shader_vs = renderctx.CreateShader(rdev, &vsinfo);

            RShaderCreateInfo psinfo = {};
            //psinfo.isCompiled = false;
            psinfo.isCompiled = true;
            psinfo.name = "fwd_test.ps";
            psinfo.type = RG_SHADER_TYPE_PIXEL;
            shader_ps = renderctx.CreateShader(rdev, &psinfo);

			RSamplerCreateInfo samplerinfo = {};
#if 0
			samplerinfo.addressModeU  = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeV  = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeW  = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
#endif
			samplerinfo.addressModeU  = RG_SAMPLER_ADDRESSMODE_REPEAT;
			samplerinfo.addressModeV  = RG_SAMPLER_ADDRESSMODE_REPEAT;
			samplerinfo.addressModeW  = RG_SAMPLER_ADDRESSMODE_REPEAT;
			samplerinfo.filterMode    = RG_SAMPLER_FILTER_LINEAR;
			samplerinfo.maxAnisotropy = 1;
			sampler_linear = renderctx.CreateSampler(rdev, &samplerinfo);


            CreateFramebuffers();

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

            renderctx.ImGui_Shutdown(rdev);

			renderctx.DestroyCommandBuffer(cmdbuffer);

            renderctx.DestroyShader(skinning_shader);

			renderctx.DestroySampler(sampler_linear);
            renderctx.DestroyShader(shader_vs);
            renderctx.DestroyShader(shader_ps);


            DestroyFramebuffers();
            DestroyMaterials();
            DestroyTextures();

            renderctx.DestroyDevice(rdev);

            imguicallbacks.clear();
        }

        void RegisterImGuiDrawCallback(RenderImGuiCallback cb) {
            imguicallbacks.push_back(cb);
        }
        
        void FreeImGuiDrawCallback(RenderImGuiCallback cb) {
            std::vector<RenderImGuiCallback>::iterator it;
            for (it = imguicallbacks.begin(); it != imguicallbacks.end(); it++) {
                if (*it == cb) {
                    *it = std::move(imguicallbacks.back());
                    imguicallbacks.pop_back();
                    break;
                }
            }
        }

        SDL_Window* ShowWindow(Uint32 w, Uint32 h) {
            return renderctx.ShowWindow(w, h);
        }

        void SwapBuffers() {
            RSwapBuffersInfo sbinfo = {};

            if (isWindowResized) {
                // Free swapchain resources
                DestroyFramebuffers();

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
                CreateFramebuffers();
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
			mat4_view(&view, info->position, info->rotation);
            mat_camera.viewproj = info->projection * view;

            //renderctx.R3D_SetCamera(info);
        }

        void UpdateSystems() {
            particlesystem->UpdateComponents(NULL);
        }

        static void DrawStatic(RCommandBuffer* cmdbuf, R3D_StaticModel* mdl) {
            R3D_Material* current_mat = NULL;
			Bool useMaterial = true;

            // Bind vertexbuffer
            renderctx.CmdBindVertexBuffer(cmdbuffer, mdl->vBuffer, 0, sizeof(R3D_Vertex));
            renderctx.CmdBindIndexBuffer(cmdbuffer, mdl->iBuffer, mdl->iType);

            renderctx.CmdPushConstants(cmdbuffer, &mat_camera, sizeof(mat_transform), RG_SHADER_TYPE_VERTEX);

            for (Uint32 i = 0; i < mdl->mCount; i++) {
                R3D_MeshInfo* minfo = &mdl->info[i];
                R3D_Material* mat   = minfo->material;

                // Bind material
                if (useMaterial && current_mat != mat) {

                    vec4 color = { mat->color.r, mat->color.g, mat->color.b, 1 };

                    renderctx.CmdPushConstants(cmdbuffer, &color, sizeof(vec4), RG_SHADER_TYPE_PIXEL);

                    // Bind textures
					RBindResourceViewInfo info[3] = {};
                    info[0].rv     = mat->albedo->srv ? mat->albedo->srv : GetDefaultWhiteTexture()->srv;
                    info[0].slot   = 0;
                    info[0].target = RG_PIPELINE_TYPE_GRAPHICS;
                    info[0].type   = RG_RESOURCEVIEW_TYPE_SRV;
                    info[1].rv     = mat->normal->srv ? mat->normal->srv : GetDefaultNormalTexture()->srv;
                    info[1].slot   = 1;
                    info[1].target = RG_PIPELINE_TYPE_GRAPHICS;
                    info[1].type   = RG_RESOURCEVIEW_TYPE_SRV;
                    info[2].rv     = mat->pbr->srv ? mat->pbr->srv : GetDefaultPBRTexture()->srv;
                    info[2].slot   = 2;
                    info[2].target = RG_PIPELINE_TYPE_GRAPHICS;
					info[2].type   = RG_RESOURCEVIEW_TYPE_SRV;
					renderctx.CmdBindResourceViews(cmdbuffer, 3, info);
                }

				// Draw mesh
                renderctx.CmdDrawIndexed(cmdbuffer, minfo->indexCount, minfo->indexOffset);
            }

		}

        void Update() {
			World* world = Engine::GetWorld();
            ModelSystem* mdlsystem = GetModelSystem();

            UpdateFrametime(GetDeltaTime());
            // Render scene
            // TODO

#if 1
            {
                renderctx.ResetCommandBuffer(cmdbuffer);
                renderctx.BeginCommandBuffer(cmdbuffer);

                // Calculate skeleton animations
                renderctx.CmdBindPipeline(cmdbuffer, skinning_pipeline);
                for (Uint32 i = 0; i < mdlsystem->GetRiggedModelCount(); i++) {
                    RiggedModelComponent* com = mdlsystem->GetRiggedModelComponent(i);
                    R3D_BoneBuffer* bbuf = com->GetKinematicsModel()->GetBufferHandle();
                    R3D_RiggedModel* mdl = com->GetHandle(); //->s_model.iCount

                    RBindResourceViewInfo info[4] = {};
                    info[0].rv = bbuf->rv;
                    info[0].slot = 0;
                    info[0].target = RG_PIPELINE_TYPE_COMPUTE;
                    info[0].type = RG_RESOURCEVIEW_TYPE_SRV;
                    info[1].rv = mdl->i_srv_vtx;
                    info[1].slot = 1;
                    info[1].target = RG_PIPELINE_TYPE_COMPUTE;
                    info[1].type = RG_RESOURCEVIEW_TYPE_SRV;
                    info[2].rv = mdl->i_srv_wht;
                    info[2].slot = 2;
                    info[2].target = RG_PIPELINE_TYPE_COMPUTE;
                    info[2].type = RG_RESOURCEVIEW_TYPE_SRV;
                    info[3].rv = mdl->s_uav;
                    info[3].slot = 3;
                    info[3].target = RG_PIPELINE_TYPE_COMPUTE;
                    info[3].type = RG_RESOURCEVIEW_TYPE_UAV;
                    renderctx.CmdBindResourceViews(cmdbuffer, 4, info);

                    renderctx.CmdDispatch(cmdbuffer, mdl->vCount, 1, 1);
                }

                renderctx.EndCommandBuffer(cmdbuffer);

                RCommandBufferSubmitInfo submitinfo = {};
                submitinfo.buffer = cmdbuffer;
                renderctx.SubmitCommandBuffer(&submitinfo);

            }

            {
                renderctx.ResetCommandBuffer(cmdbuffer);
                renderctx.BeginCommandBuffer(cmdbuffer);

				// Draw 3D scene
				RRenderpassClearInfo clearinfo = {};
                clearinfo.color[0] = { 0, 0, 0, 1 };
                clearinfo.depth    = 1.0f;
                clearinfo.stencil  = 0;
                RRenderpassBeginInfo rpbegininfo = {};
				rpbegininfo.framebuffer = framebuffer3d[frameIndex];
                rpbegininfo.renderpass  = renderpass3d;
                rpbegininfo.clearinfo   = &clearinfo;
                renderctx.CmdBeginRenderpass(cmdbuffer, &rpbegininfo);
                renderctx.CmdBindPipeline(cmdbuffer, pipeline3d);
				renderctx.CmdBindSampler(cmdbuffer, sampler_linear, 3, RG_SHADER_TYPE_PIXEL);

				// Draw entities

                for (Uint32 i = 0; i < world->GetEntityCount(); i++) {
					Entity* ent = world->GetEntity(i);
                    ModelComponent* mc = ent->GetComponent(Component_MODELCOMPONENT)->AsModelComponent();
                    RiggedModelComponent* rmc = ent->GetComponent(Component_RIGGEDMODELCOMPONENT)->AsRiggedModelComponent();

                    mat_camera.model = *ent->GetTransform()->GetMatrix();

                    if (mc) {
                        DrawStatic(cmdbuffer, mc->GetHandle());
                    }

                    if (rmc) {
                        DrawStatic(cmdbuffer, &rmc->GetHandle()->s_model);
                    }

                }


                // Draw static models

                for (Uint32 i = 0; i < world->GetStaticCount(); i++) {
                    StaticObject* staticobj = world->GetStaticObject(i);

                    mat_camera.model = *staticobj->GetMatrix();
                    R3D_StaticModel* staticmdl = staticobj->GetModelHandle();

					DrawStatic(cmdbuffer, staticmdl);
                }


                renderctx.CmdEndRenderpass(cmdbuffer);

                renderctx.EndCommandBuffer(cmdbuffer);

                RCommandBufferSubmitInfo submitinfo = {};
                submitinfo.buffer = cmdbuffer;
                renderctx.SubmitCommandBuffer(&submitinfo);
            }
#endif
            // Update ImGui
            renderctx.ImGui_NewFrame(rdev);
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // Call all registered callbacks
            std::vector<RenderImGuiCallback>::iterator it;
            for (it = imguicallbacks.begin(); it != imguicallbacks.end(); it++) {
                RenderImGuiCallback cb = *it;
                cb();
            }

            ImGui::EndFrame();
            ImGui::Render();

            {
                // Draw imgui
                renderctx.ResetCommandBuffer(cmdbuffer);
                renderctx.BeginCommandBuffer(cmdbuffer);
                {
                    renderctx.CmdBeginRenderpass(cmdbuffer, NULL);
                    renderctx.CmdImGuiRenderDrawData(cmdbuffer, ImGui::GetDrawData());
                    renderctx.CmdEndRenderpass(cmdbuffer);
                }
                renderctx.EndCommandBuffer(cmdbuffer);

                RCommandBufferSubmitInfo submitinfo = {};
                submitinfo.buffer = cmdbuffer;
                renderctx.SubmitCommandBuffer(&submitinfo);
            }

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

			// Resource views for input buffers
			RResourceViewCreateInfo rvinfo = {};
			rvinfo.type        = RG_RESOURCEVIEW_TYPE_SRV;
            rvinfo.stage       = RG_SHADER_TYPE_COMPUTE;
            rvinfo.buffer_type = RG_RESOURCEVIEW_BUFFER;
			rvinfo.elements    = info->vCount;

            rvinfo.dst_buffer = rigmdl->i_vertex;
            rigmdl->i_srv_vtx = renderctx.CreateResourceView(rdev, &rvinfo);
            rvinfo.dst_buffer = rigmdl->i_weight;
            rigmdl->i_srv_wht = renderctx.CreateResourceView(rdev, &rvinfo);

			// Resource view for output vertex buffer
            rvinfo = {};
            rvinfo.type        = RG_RESOURCEVIEW_TYPE_UAV;
            rvinfo.stage       = RG_SHADER_TYPE_COMPUTE;
            rvinfo.buffer_type = RG_RESOURCEVIEW_BUFFER;
            rvinfo.elements    = info->vCount;
            rvinfo.dst_buffer  = rigmdl->s_model.vBuffer;

			rigmdl->s_uav = renderctx.CreateResourceView(rdev, &rvinfo);

            return rigmdl;
        }

        void DestroyRiggedModel(R3D_RiggedModel* mdl) {
            if (!isRendererLoaded) { return; }

            for (Uint32 i = 0; i < mdl->s_model.mCount; i++) {
                FreeMaterial(mdl->s_model.info[i].material);
            }
            
			renderctx.DestroyResourceView(mdl->i_srv_vtx);
			renderctx.DestroyResourceView(mdl->i_srv_wht);
			renderctx.DestroyResourceView(mdl->s_uav);
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

            RResourceViewCreateInfo rvinfo = {};
            rvinfo.type        = RG_RESOURCEVIEW_TYPE_SRV;
            rvinfo.stage       = RG_SHADER_TYPE_COMPUTE;
            rvinfo.buffer_type = RG_RESOURCEVIEW_BUFFER;
            rvinfo.elements    = info->len / sizeof(mat4);
            rvinfo.dst_buffer  = bonebuf->buffer;

            bonebuf->rv = renderctx.CreateResourceView(rdev, &rvinfo);

            return bonebuf;
        }

        void DestroyBoneBuffer(R3D_BoneBuffer* hbuff) {
            if (!isRendererLoaded) { return; }

            renderctx.DestroyResourceView(hbuff->rv);
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