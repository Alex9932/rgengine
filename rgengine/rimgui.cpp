#define DLL_EXPORT
#include "rimgui.h"

#include "render.h"
#include "guiwnds.h"

#include "rpostprocess.h"

#include <vector>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"

namespace Engine {
	namespace Render {

        static float quad_verts[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
        static Uint16 quad_inds[] = { 0, 3, 2, 2, 1, 0 };

        static RBuffer* vb;
        static RBuffer* ib;
        static RSampler* sampler;

        static RShader* vs;
        static RShader* ps;
        static RPipeline* pipeline;

        static std::vector<RenderImGuiCallback> imguicallbacks;

        static RCommandBuffer* cmdbuffer = NULL;

        void CreatePipeline() {
            RRenderDevice* dev = GetRenderDevice();
            RenderBackend* ctx = GetRenderContext();

            RPipelineInputDescription description = {};
            description.format = RG_FORMAT_R32G32_FLOAT;
            description.inputSlot = 0;
            description.name = "POSITION";

            RPipelineLayoutDescription layout = {};
            layout.bindings[0].set = 0;
            layout.bindings[0].binding = 0;
            layout.bindings[0].stage = RG_SHADER_TYPE_PIXEL;
            layout.bindings[0].type = RG_DESCRIPTOR_TYPE_SAMPLER;
            layout.bindings[1].set = 1;
            layout.bindings[1].binding = 0;
            layout.bindings[1].stage = RG_SHADER_TYPE_PIXEL;
            layout.bindings[1].type = RG_DESCRIPTOR_TYPE_IMAGE;
            layout.bindings[2].set = 2;
            layout.bindings[2].binding = 0;
            layout.bindings[2].stage = RG_SHADER_TYPE_PIXEL;
            layout.bindings[2].type = RG_DESCRIPTOR_TYPE_IMAGE;
            layout.binding_count = 3;

            RPipelineCreateInfo plinfo = {};
            plinfo.type = RG_PIPELINE_TYPE_GRAPHICS;
            plinfo.cullmode = RG_RENDERPASS_CULLMODE_NONE;
            plinfo.fillmode = RG_RENDERPASS_FILLMODE_SOLID;
            plinfo.inputCount = 1;
            plinfo.vertex_shader = vs;
            plinfo.pixel_shader = ps;
            plinfo.renderpass = NULL;
            plinfo.descriptions = &description;
            plinfo.layout = &layout;
            pipeline = ctx->CreatePipeline(dev, &plinfo);
        }

        void InitRImGui() {
            RRenderDevice* dev = GetRenderDevice();
            RenderBackend* ctx = GetRenderContext();

            RCommandBufferCreateInfo cmdbuffinfo = {};
            cmdbuffinfo.maxcmds = 128;
            cmdbuffer = ctx->CreateCommandBuffer(dev, &cmdbuffinfo);

            ctx->ImGui_Init(dev);

            RBufferCreateInfo vbinfo = {};
            vbinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
            vbinfo.usage = RG_BUFFER_USAGE_DEFAULT;
            vbinfo.type = RG_BUFFER_TYPE_VERTEX;
            vbinfo.stride = sizeof(Float32) * 3;
            vbinfo.length = sizeof(quad_verts);
            vbinfo.initialData = quad_verts;
            vb = ctx->CreateBuffer(dev, &vbinfo);

            RBufferCreateInfo ibinfo = {};
            ibinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
            ibinfo.usage = RG_BUFFER_USAGE_DEFAULT;
            ibinfo.type = RG_BUFFER_TYPE_INDEX;
            ibinfo.stride = sizeof(Uint16);
            ibinfo.length = sizeof(quad_inds);
            ibinfo.initialData = quad_inds;
            ib = ctx->CreateBuffer(dev, &ibinfo);

            RShaderCreateInfo sinfo = {};
            sinfo.isCompiled = true;
            sinfo.type = RG_SHADER_TYPE_VERTEX;
            sinfo.name = "output.vs";
            vs = ctx->CreateShader(dev, &sinfo);
            sinfo.type = RG_SHADER_TYPE_PIXEL;
            sinfo.name = "output.ps";
            ps = ctx->CreateShader(dev, &sinfo);

            RSamplerCreateInfo samplerinfo = {};
            samplerinfo.addressModeU = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
            samplerinfo.addressModeV = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
            samplerinfo.addressModeW = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
            samplerinfo.filterMode = RG_SAMPLER_FILTER_LINEAR;
            samplerinfo.maxAnisotropy = 1;
            sampler = ctx->CreateSampler(dev, &samplerinfo);

            CreatePipeline();
        }

        void DestroyRImGui() {
            RRenderDevice* dev = GetRenderDevice();
            RenderBackend* ctx = GetRenderContext();
            ctx->DestroyCommandBuffer(cmdbuffer);
            ctx->ImGui_Shutdown(dev);

			ctx->DestroyPipeline(pipeline);
			ctx->DestroyBuffer(vb);
            ctx->DestroyBuffer(ib);
            ctx->DestroySampler(sampler);
            ctx->DestroyShader(vs);
            ctx->DestroyShader(ps);
        }

        void ResizeRImGui() {
            RenderBackend* ctx = GetRenderContext();
            ctx->DestroyPipeline(pipeline);
            CreatePipeline();
        }

		void UpdateImGui() {
            RRenderDevice* dev = GetRenderDevice();
            RenderBackend* ctx = GetRenderContext();

            // Update ImGui
            ctx->ImGui_NewFrame(dev);
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // Call all registered callbacks
            std::vector<RenderImGuiCallback>::iterator it;
            for (it = imguicallbacks.begin(); it != imguicallbacks.end(); it++) {
                RenderImGuiCallback cb = *it;
                cb();
            }

            DrawRendererStats();

            ImGui::EndFrame();
            ImGui::Render();
		}

        void DrawImGui(Uint32 drawOutput) {
            RenderBackend* ctx = GetRenderContext();

            // Draw imgui
            ctx->ResetCommandBuffer(cmdbuffer);
            ctx->BeginCommandBuffer(cmdbuffer);

            ctx->CmdBeginRenderpass(cmdbuffer, NULL);

            // Draw output image
            if (drawOutput) {

                ctx->CmdUseImage(cmdbuffer, GetPostProcessOutputImage(), RG_IMAGE_USAGE_SHADER_READ_ONLY);
                ctx->CmdBindPipeline(cmdbuffer, pipeline);
                ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
                ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
                RBindDescriptorSetsInfo setinfo = {};
                setinfo.count = 1;
                setinfo.startslot = 1;
                RDescriptorSet* descset_outputimage = GetPostProcessOutputSet();
                setinfo.sets = &descset_outputimage;
                ctx->CmdBindDescriptorSets(cmdbuffer, &setinfo);
                ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
                ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
            }

            // Draw ImGui

            ctx->CmdImGuiRenderDrawData(cmdbuffer, ImGui::GetDrawData());
            ctx->CmdEndRenderpass(cmdbuffer);

            ctx->EndCommandBuffer(cmdbuffer);

            RCommandBufferSubmitInfo submitinfo = {};
            submitinfo.buffer = cmdbuffer;
            ctx->SubmitCommandBuffer(&submitinfo);
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

	}
}