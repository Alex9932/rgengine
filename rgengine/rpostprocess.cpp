#include "rpostprocess.h"

#include "render.h"
#include "rlighting.h"

namespace Engine {
	namespace Render {

		static float quad_verts[] = { -1, -1, 0, 1, -1, 0, 1, 1, 0, -1, 1, 0 };
		static Uint16 quad_inds[] = { 0, 3, 2, 2, 1, 0 };

		static void* set; // output image for ImGui

		static RImage* rtarget = NULL;
		static RFramebuffer* framebuffer = NULL;
		static RRenderpass* renderpass = NULL;

		static RBuffer*  vb;
		static RBuffer*  ib;
		static RSampler* sampler;

		static RShader* vs;
		static RShader* ps;
		static RPipeline* pipeline;

		static RCommandBuffer* cmdbuffer;

		static void CreateFramebuffer(ivec2* size) {
			RenderBackend* ctx = GetRenderContext();
			RRenderDevice* dev = GetRenderDevice();

			RImageCreateInfo cainfo = {}; // Color attachments
			cainfo.format = RG_FORMAT_R8G8B8A8_UNORM;
			cainfo.width  = size->x;
			cainfo.height = size->y;
			rtarget = ctx->CreateImage(dev, &cainfo);

			set = ctx->ImGui_AddTexture(dev, rtarget);

			RRenderpassCreateInfo rpinfo = {};
			rpinfo.rt_count = 1;
			rpinfo.use_depth = false;
			rpinfo.viewport.x = 0;
			rpinfo.viewport.y = 0;
			rpinfo.viewport.width = size->x;
			rpinfo.viewport.height = size->y;
			rpinfo.rt_formats[0] = RG_FORMAT_R8G8B8A8_UNORM;
			renderpass = ctx->CreateRenderpass(dev, &rpinfo);

			RFramebufferCreateInfo fbinfo = {};
			fbinfo.width = size->x;
			fbinfo.height = size->y;
			fbinfo.rt_count = 1;
			fbinfo.rts[0] = rtarget;
			fbinfo.renderpass = renderpass;
			framebuffer = ctx->CreateFramebuffer(dev, &fbinfo);

			RPipelineInputDescription description = {};
			description.format = RG_FORMAT_R32G32B32_FLOAT;
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
			layout.binding_count = 2;

			RPipelineCreateInfo plinfo = {};
			plinfo.type = RG_PIPELINE_TYPE_GRAPHICS;
			plinfo.cullmode = RG_RENDERPASS_CULLMODE_NONE;
			plinfo.fillmode = RG_RENDERPASS_FILLMODE_SOLID;
			plinfo.inputCount = 1;
			plinfo.vertex_shader = vs;
			plinfo.pixel_shader = ps;
			plinfo.renderpass = renderpass;
			plinfo.descriptions = &description;
			plinfo.layout = &layout;
			pipeline = ctx->CreatePipeline(dev, &plinfo);
		}

		static void DestroyFramebuffer() {
			RenderBackend* ctx = GetRenderContext();

			ctx->DestroyPipeline(pipeline);
			ctx->DestroyFramebuffer(framebuffer);
			ctx->DestroyRenderpass(renderpass);

			ctx->ImGui_RemoveTexture(set);
			ctx->DestroyImage(rtarget);
		}

		void InitPostProcess(ivec2* size) {
			RenderBackend* ctx = GetRenderContext();
			RRenderDevice* dev = GetRenderDevice();

			RCommandBufferCreateInfo cmdbuffinfo = {};
			cmdbuffinfo.maxcmds = 128;
			cmdbuffer = ctx->CreateCommandBuffer(dev, &cmdbuffinfo);

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

			CreateFramebuffer(size);
		}

		void DestroyPostProcess() {
			RenderBackend* ctx = GetRenderContext();
			DestroyFramebuffer();

			ctx->DestroyBuffer(vb);
			ctx->DestroyBuffer(ib);
			ctx->DestroyShader(vs);
			ctx->DestroyShader(ps);
			ctx->DestroySampler(sampler);
			ctx->DestroyCommandBuffer(cmdbuffer);
		}

		void ResizePostProcess(ivec2* size) {
			DestroyFramebuffer();
			CreateFramebuffer(size);
		}

		void DoPostProcess() {
			RenderBackend* ctx = GetRenderContext();

			ctx->ResetCommandBuffer(cmdbuffer);
			ctx->BeginCommandBuffer(cmdbuffer);

			ctx->CmdUseImage(cmdbuffer, rtarget, RG_IMAGE_USAGE_COLOR_ATTACHMENT);

			ctx->CmdUseImage(cmdbuffer, GetRLightingOutput(), RG_IMAGE_USAGE_SHADER_READ_ONLY);

			RRenderpassClearInfo clear = {};
			clear.color[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
			RRenderpassBeginInfo rpinfo = {};
			rpinfo.clearinfo = &clear;
			rpinfo.framebuffer = framebuffer;
			rpinfo.renderpass = renderpass;

			ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);

			ctx->CmdBindPipeline(cmdbuffer, pipeline);
			ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 3);
			ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);

			RBindDescriptorSetsInfo info = {};
			RDescriptorSet* gbufferset = GetRLightingOutputSet();
			info.sets = &gbufferset;
			info.startslot = 1;
			info.count = 1;
			ctx->CmdBindDescriptorSets(cmdbuffer, &info);
			ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);

			ctx->CmdDrawIndexed(cmdbuffer, 6, 0);

			ctx->CmdEndRenderpass(cmdbuffer);
			ctx->CmdUseImage(cmdbuffer, rtarget, RG_IMAGE_USAGE_SHADER_READ_ONLY);
			ctx->EndCommandBuffer(cmdbuffer);

			RCommandBufferSubmitInfo submitinfo = {};
			submitinfo.buffer = cmdbuffer;
			ctx->SubmitCommandBuffer(&submitinfo);
		}

		void* GetPostProcessOutputSet() { return set; }

	}
}
