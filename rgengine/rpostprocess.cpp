#include "rpostprocess.h"

#include "render.h"
#include "rlighting.h"

namespace Engine {
	namespace Render {

		static float quad_verts[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
		static Uint16 quad_inds[] = { 0, 3, 2, 2, 1, 0 };

		// Blur
		static RImage* rt_blur1x;
		static RImage* rt_blur2x;
		static RImage* rt_blur3x;
		static RImage* rt_blur1y;
		static RImage* rt_blur2y;
		static RImage* rt_blur3y;
		static RFramebuffer* fb_blur1x;
		static RFramebuffer* fb_blur2x;
		static RFramebuffer* fb_blur3x;
		static RFramebuffer* fb_blur1y;
		static RFramebuffer* fb_blur2y;
		static RFramebuffer* fb_blur3y;
		static RRenderpass* rp_blur1;
		static RRenderpass* rp_blur2;
		static RRenderpass* rp_blur3;
		static RDescriptorSet* ds_blur1x;
		static RDescriptorSet* ds_blur2x;
		static RDescriptorSet* ds_blur3x;
		static RDescriptorSet* ds_blur1y;
		static RDescriptorSet* ds_blur2y;
		static RDescriptorSet* ds_blur3y;

		static ivec2 blur_p1;
		static ivec2 blur_p2;
		static ivec2 blur_p3;

		static RShader* vs_blur;
		static RShader* ps_blur;
		static RPipeline* pl_blur1;
		static RPipeline* pl_blur2;
		static RPipeline* pl_blur3;

		static void* imguiset; // output image for ImGui
		static RDescriptorSet* set;

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

			imguiset = ctx->ImGui_AddTexture(dev, rtarget);

			RDescriptorSetBinding dsbinding = {};
			dsbinding.binding = 0;
			dsbinding.stage = RG_SHADER_TYPE_PIXEL;
			dsbinding.type = RG_DESCRIPTOR_TYPE_IMAGE;
			dsbinding.image = rtarget;
			RDescriptorSetCreateInfo dsinfo = {};
			dsinfo.bindings = &dsbinding;
			dsinfo.binding_count = 1;
			set = ctx->CreateDescriptorSet(dev, &dsinfo);

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
			plinfo.renderpass = renderpass;
			plinfo.descriptions = &description;
			plinfo.layout = &layout;
			pipeline = ctx->CreatePipeline(dev, &plinfo);


			blur_p1 = { size->x, size->y };
			blur_p2 = { size->x / 8, size->y / 8 };
			blur_p3 = { size->x / 64, size->y / 64 };


			cainfo.format = RG_FORMAT_R16G16B16A16_FLOAT;
			cainfo.width  = blur_p1.x;
			cainfo.height = blur_p1.y;
			rt_blur1x = ctx->CreateImage(dev, &cainfo);
			rt_blur1y = ctx->CreateImage(dev, &cainfo);
			cainfo.width  = blur_p2.x;
			cainfo.height = blur_p2.y;
			rt_blur2x = ctx->CreateImage(dev, &cainfo);
			rt_blur2y = ctx->CreateImage(dev, &cainfo);
			cainfo.width  = blur_p3.x;
			cainfo.height = blur_p3.y;
			rt_blur3x = ctx->CreateImage(dev, &cainfo);
			rt_blur3y = ctx->CreateImage(dev, &cainfo);

			RDescriptorSetBinding binding = {};
			RDescriptorSetCreateInfo ds_info = {};
			ds_info.binding_count = 1;
			ds_info.bindings = &binding;
			binding.binding = 0;
			binding.stage = RG_SHADER_TYPE_PIXEL;
			binding.type  = RG_DESCRIPTOR_TYPE_IMAGE;
			binding.image = rt_blur1x;
			ds_blur1x = ctx->CreateDescriptorSet(dev, &ds_info);
			binding.image = rt_blur2x;
			ds_blur2x = ctx->CreateDescriptorSet(dev, &ds_info);
			binding.image = rt_blur3x;
			ds_blur3x = ctx->CreateDescriptorSet(dev, &ds_info);
			binding.image = rt_blur1y;
			ds_blur1y = ctx->CreateDescriptorSet(dev, &ds_info);
			binding.image = rt_blur2y;
			ds_blur2y = ctx->CreateDescriptorSet(dev, &ds_info);
			binding.image = rt_blur3y;
			ds_blur3y = ctx->CreateDescriptorSet(dev, &ds_info);


			rpinfo.rt_formats[0] = RG_FORMAT_R16G16B16A16_FLOAT;
			rpinfo.rt_count = 1;
			rpinfo.use_depth = false;
			rpinfo.viewport.x = 0;
			rpinfo.viewport.y = 0;
			rpinfo.viewport.width  = blur_p1.x;
			rpinfo.viewport.height = blur_p1.y;
			rp_blur1 = ctx->CreateRenderpass(dev, &rpinfo);
			rpinfo.viewport.width  = blur_p2.x;
			rpinfo.viewport.height = blur_p2.y;
			rp_blur2 = ctx->CreateRenderpass(dev, &rpinfo);
			rpinfo.viewport.width  = blur_p3.x;
			rpinfo.viewport.height = blur_p3.y;
			rp_blur3 = ctx->CreateRenderpass(dev, &rpinfo);


			fbinfo.rt_count = 1;
			fbinfo.width  = blur_p1.x;
			fbinfo.height = blur_p1.y;
			fbinfo.renderpass = rp_blur1;
			fbinfo.rts[0] = rt_blur1x;
			fb_blur1x = ctx->CreateFramebuffer(dev, &fbinfo);
			fbinfo.rts[0] = rt_blur1y;
			fb_blur1y = ctx->CreateFramebuffer(dev, &fbinfo);
			
			fbinfo.width  = blur_p2.x;
			fbinfo.height = blur_p2.y;
			fbinfo.renderpass = rp_blur2;
			fbinfo.rts[0] = rt_blur2x;
			fb_blur2x = ctx->CreateFramebuffer(dev, &fbinfo);
			fbinfo.rts[0] = rt_blur2y;
			fb_blur2y = ctx->CreateFramebuffer(dev, &fbinfo);
			
			fbinfo.width  = blur_p3.x;
			fbinfo.height = blur_p3.y;
			fbinfo.renderpass = rp_blur3;
			fbinfo.rts[0] = rt_blur3x;
			fb_blur3x = ctx->CreateFramebuffer(dev, &fbinfo);
			fbinfo.rts[0] = rt_blur3y;
			fb_blur3y = ctx->CreateFramebuffer(dev, &fbinfo);


			layout.binding_count = 2;

			plinfo.type = RG_PIPELINE_TYPE_GRAPHICS;
			plinfo.cullmode = RG_RENDERPASS_CULLMODE_NONE;
			plinfo.fillmode = RG_RENDERPASS_FILLMODE_SOLID;
			plinfo.inputCount = 1;
			plinfo.vertex_shader = vs_blur;
			plinfo.pixel_shader = ps_blur;
			plinfo.renderpass = rp_blur1;
			plinfo.descriptions = &description;
			plinfo.layout = &layout;
			pl_blur1 = ctx->CreatePipeline(dev, &plinfo);
			plinfo.renderpass = rp_blur2;
			pl_blur2 = ctx->CreatePipeline(dev, &plinfo);
			plinfo.renderpass = rp_blur3;
			pl_blur3 = ctx->CreatePipeline(dev, &plinfo);

		}

		static void DestroyFramebuffer() {
			RenderBackend* ctx = GetRenderContext();

			ctx->DestroyFramebuffer(fb_blur1x);
			ctx->DestroyFramebuffer(fb_blur2x);
			ctx->DestroyFramebuffer(fb_blur3x);
			ctx->DestroyFramebuffer(fb_blur1y);
			ctx->DestroyFramebuffer(fb_blur2y);
			ctx->DestroyFramebuffer(fb_blur3y);
			ctx->DestroyRenderpass(rp_blur1);
			ctx->DestroyRenderpass(rp_blur2);
			ctx->DestroyRenderpass(rp_blur3);
			ctx->DestroyDescriptorSet(ds_blur1x);
			ctx->DestroyDescriptorSet(ds_blur2x);
			ctx->DestroyDescriptorSet(ds_blur3x);
			ctx->DestroyDescriptorSet(ds_blur1y);
			ctx->DestroyDescriptorSet(ds_blur2y);
			ctx->DestroyDescriptorSet(ds_blur3y);
			ctx->DestroyImage(rt_blur1x);
			ctx->DestroyImage(rt_blur2x);
			ctx->DestroyImage(rt_blur3x);
			ctx->DestroyImage(rt_blur1y);
			ctx->DestroyImage(rt_blur2y);
			ctx->DestroyImage(rt_blur3y);

			ctx->DestroyPipeline(pl_blur1);
			ctx->DestroyPipeline(pl_blur2);
			ctx->DestroyPipeline(pl_blur3);

			ctx->DestroyPipeline(pipeline);
			ctx->DestroyFramebuffer(framebuffer);
			ctx->DestroyRenderpass(renderpass);

			ctx->ImGui_RemoveTexture(imguiset);
			ctx->DestroyDescriptorSet(set);
			ctx->DestroyImage(rtarget);
		}

		static void LoadShaders() {
			RenderBackend* ctx = GetRenderContext();
			RRenderDevice* dev = GetRenderDevice();
			RShaderCreateInfo sinfo = {};
			sinfo.isCompiled = true;
			sinfo.type = RG_SHADER_TYPE_VERTEX;
			sinfo.name = "combine.vs";
			vs = ctx->CreateShader(dev, &sinfo);
			sinfo.type = RG_SHADER_TYPE_PIXEL;
			sinfo.name = "combine.ps";
			ps = ctx->CreateShader(dev, &sinfo);

			sinfo.type = RG_SHADER_TYPE_VERTEX;
			sinfo.name = "blur.vs";
			vs_blur = ctx->CreateShader(dev, &sinfo);
			sinfo.type = RG_SHADER_TYPE_PIXEL;
			sinfo.name = "blur.ps";
			ps_blur = ctx->CreateShader(dev, &sinfo);
		}

		static void DestroyShaders() {
			RenderBackend* ctx = GetRenderContext();
			ctx->DestroyShader(vs_blur);
			ctx->DestroyShader(ps_blur);
			ctx->DestroyShader(vs);
			ctx->DestroyShader(ps);
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

			RSamplerCreateInfo samplerinfo = {};
			samplerinfo.addressModeU = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeV = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeW = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.filterMode = RG_SAMPLER_FILTER_LINEAR;
			samplerinfo.maxAnisotropy = 1;
			sampler = ctx->CreateSampler(dev, &samplerinfo);

			LoadShaders();
			CreateFramebuffer(size);
		}

		void DestroyPostProcess() {
			RenderBackend* ctx = GetRenderContext();
			DestroyFramebuffer();
			DestroyShaders();
			ctx->DestroyBuffer(vb);
			ctx->DestroyBuffer(ib);
			ctx->DestroySampler(sampler);
			ctx->DestroyCommandBuffer(cmdbuffer);
		}

		void ResizePostProcess(ivec2* size) {
			DestroyFramebuffer();
			CreateFramebuffer(size);
		}

		void ReloadPostProcess(ivec2* size) {
			DestroyShaders();
			LoadShaders();
			ResizePostProcess(size);
		}

		void DoPostProcess() {
			RenderBackend* ctx = GetRenderContext();

			ctx->ResetCommandBuffer(cmdbuffer);
			ctx->BeginCommandBuffer(cmdbuffer);

			{
				RRenderpassClearInfo clear = {};
				clear.color[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
				RRenderpassBeginInfo rpinfo = {};
				rpinfo.clearinfo = &clear;

				// Blur downscale pass 1
				ctx->CmdUseImage(cmdbuffer, rt_blur1x, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				ctx->CmdUseImage(cmdbuffer, GetRLightingOutput(), RG_IMAGE_USAGE_SHADER_READ_ONLY);

				rpinfo.framebuffer = fb_blur1x;
				rpinfo.renderpass = rp_blur1;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);

				ctx->CmdBindPipeline(cmdbuffer, pl_blur1);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				RBindDescriptorSetsInfo info = {};
				RDescriptorSet* gbufferset = GetRLightingOutputSet();
				info.sets = &gbufferset;
				info.startslot = 1;
				info.count = 1;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				// x = 2 - for apply threshold
				vec4 axis = { 2.0f, 0.0f, (Float32)blur_p1.x, (Float32)blur_p1.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);

				ctx->CmdUseImage(cmdbuffer, rt_blur1x, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur1y, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur1y;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur1);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur1x;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 0.0f, 1.0f, (Float32)blur_p1.x, (Float32)blur_p1.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);


				// Blur downscale pass 2
				ctx->CmdUseImage(cmdbuffer, rt_blur1y, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur2x, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur2x;
				rpinfo.renderpass = rp_blur2;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur2);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur1y;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 1.0f, 0.0f, (Float32)blur_p2.x, (Float32)blur_p2.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);

				ctx->CmdUseImage(cmdbuffer, rt_blur2x, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur2y, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur2y;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur2);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur2x;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 0.0f, 1.0f, (Float32)blur_p2.x, (Float32)blur_p2.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);


				// Blur downscale pass 3
				ctx->CmdUseImage(cmdbuffer, rt_blur2y, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur3x, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur3x;
				rpinfo.renderpass = rp_blur3;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur3);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur2y;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 1.0f, 0.0f, (Float32)blur_p3.x, (Float32)blur_p3.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);

				ctx->CmdUseImage(cmdbuffer, rt_blur3x, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur3y, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur3y;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur3);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur3x;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 0.0f, 1.0f, (Float32)blur_p3.x, (Float32)blur_p3.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);


				// Blur upscale pass 1
				ctx->CmdUseImage(cmdbuffer, rt_blur3y, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur2x, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur2x;
				rpinfo.renderpass = rp_blur2;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur2);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur3y;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 1.0f, 0.0f, (Float32)blur_p2.x, (Float32)blur_p2.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);

				ctx->CmdUseImage(cmdbuffer, rt_blur2x, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur2y, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur2y;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur2);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur2x;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 0.0f, 1.0f, (Float32)blur_p2.x, (Float32)blur_p2.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);


				// Blur upscale pass 2

				ctx->CmdUseImage(cmdbuffer, rt_blur2y, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur1x, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur1x;
				rpinfo.renderpass = rp_blur1;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur1);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur2y;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 1.0f, 0.0f, (Float32)blur_p1.x, (Float32)blur_p1.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);

				ctx->CmdUseImage(cmdbuffer, rt_blur1x, RG_IMAGE_USAGE_SHADER_READ_ONLY);
				ctx->CmdUseImage(cmdbuffer, rt_blur1y, RG_IMAGE_USAGE_COLOR_ATTACHMENT);
				rpinfo.framebuffer = fb_blur1y;
				ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
				ctx->CmdBindPipeline(cmdbuffer, pl_blur1);
				ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
				ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);
				info.sets = &ds_blur1x;
				ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);
				axis = { 0.0f, 1.0f, (Float32)blur_p1.x, (Float32)blur_p1.y };
				ctx->CmdPushConstants(cmdbuffer, &axis, sizeof(vec4), RG_SHADER_TYPE_PIXEL);
				ctx->CmdDrawIndexed(cmdbuffer, 6, 0);
				ctx->CmdEndRenderpass(cmdbuffer);


			}

			ctx->CmdUseImage(cmdbuffer, rtarget, RG_IMAGE_USAGE_COLOR_ATTACHMENT);

			ctx->CmdUseImage(cmdbuffer, GetRLightingOutput(), RG_IMAGE_USAGE_SHADER_READ_ONLY);
			ctx->CmdUseImage(cmdbuffer, rt_blur1y, RG_IMAGE_USAGE_SHADER_READ_ONLY);

			RRenderpassClearInfo clear = {};
			clear.color[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
			RRenderpassBeginInfo rpinfo = {};
			rpinfo.clearinfo = &clear;
			rpinfo.framebuffer = framebuffer;
			rpinfo.renderpass = renderpass;

			ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);

			ctx->CmdBindPipeline(cmdbuffer, pipeline);
			ctx->CmdBindVertexBuffer(cmdbuffer, vb, 0, sizeof(Float32) * 2);
			ctx->CmdBindIndexBuffer(cmdbuffer, ib, RG_INDEX_U16);

			RBindDescriptorSetsInfo info = {};
			RDescriptorSet* gbufferset = GetRLightingOutputSet();
			info.sets = &gbufferset;
			info.startslot = 1;
			info.count = 1;
			ctx->CmdBindDescriptorSets(cmdbuffer, &info);
			info.sets = &ds_blur1y;
			info.startslot = 2;
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

		void* GetPostProcessOutputImGuiSet() { return imguiset; }
		RDescriptorSet* GetPostProcessOutputSet() { return set; }
		RImage* GetPostProcessOutputImage() { return rtarget; }

	}
}
