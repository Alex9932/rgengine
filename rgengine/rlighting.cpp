#include "rlighting.h"

#include "render.h"
#include "rgbuffer.h"

#include "rgvector.h"

namespace Engine {
	namespace Render {

		static RShader* vs;
		static RShader* ps;

		static RSampler* sampler;
		static RPipeline* pipeline;
		static RCommandBuffer* cmdbuffer;
		static void* set;

		static RImage*       rtarget = NULL;
		static RFramebuffer* framebuffer = NULL;
		static RRenderpass*  renderpass = NULL;

		static float verts[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
		static Uint16 inds[] = { 0, 3, 2, 2, 1, 0 };

		static RBuffer* vertexbuffer;
		static RBuffer* indexbuffer;

		static void CreateFramebuffer(ivec2* size) {
			RenderBackend* ctx = GetRenderContext();
			RRenderDevice* dev = GetRenderDevice();

			RImageCreateInfo cainfo = {}; // Color attachments
			cainfo.format = RG_FORMAT_R8G8B8A8_UNORM;
			cainfo.width  = size->x;
			cainfo.height = size->y;
			rtarget = ctx->CreateImage(dev, &cainfo);

			set = ctx->ImGui_AddTexture(dev, rtarget);

			RRenderpassCreateInfo rp3dinfo = {};
			rp3dinfo.rt_count = 1;
			rp3dinfo.use_depth = false;
			rp3dinfo.viewport.x = 0;
			rp3dinfo.viewport.y = 0;
			rp3dinfo.viewport.width = size->x;
			rp3dinfo.viewport.height = size->y;
			rp3dinfo.rt_formats[0] = RG_FORMAT_R8G8B8A8_UNORM;
			renderpass = ctx->CreateRenderpass(dev, &rp3dinfo);

			RFramebufferCreateInfo fbinfo = {};
			fbinfo.width    = size->x;
			fbinfo.height   = size->y;
			fbinfo.rt_count = 1;
			fbinfo.rts[0]   = rtarget;
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
			layout.bindings[2].set = 1;
			layout.bindings[2].binding = 1;
			layout.bindings[2].stage = RG_SHADER_TYPE_PIXEL;
			layout.bindings[2].type = RG_DESCRIPTOR_TYPE_IMAGE;
			layout.bindings[3].set = 1;
			layout.bindings[3].binding = 2;
			layout.bindings[3].stage = RG_SHADER_TYPE_PIXEL;
			layout.bindings[3].type = RG_DESCRIPTOR_TYPE_IMAGE;
			layout.binding_count = 4;

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

			ctx->ImGui_RemoveTexture(set);
			ctx->DestroyPipeline(pipeline);
			ctx->DestroyFramebuffer(framebuffer);
			ctx->DestroyRenderpass(renderpass);
			ctx->DestroyImage(rtarget);
		}

		void InitRLighting(ivec2* size) {
			RenderBackend* ctx = GetRenderContext();
			RRenderDevice* dev = GetRenderDevice();

			RCommandBufferCreateInfo cmdbuffinfo = {};
			cmdbuffinfo.maxcmds = 128;
			cmdbuffer = ctx->CreateCommandBuffer(dev, &cmdbuffinfo);

			RBufferCreateInfo vbinfo = {};
			vbinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
			vbinfo.usage = RG_BUFFER_USAGE_DEFAULT;
			vbinfo.type = RG_BUFFER_TYPE_VERTEX;
			vbinfo.stride = sizeof(Float32) * 2;
			vbinfo.length = sizeof(verts);
			vbinfo.initialData = verts;
			vertexbuffer = ctx->CreateBuffer(dev, &vbinfo);

			RBufferCreateInfo ibinfo = {};
			ibinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
			ibinfo.usage = RG_BUFFER_USAGE_DEFAULT;
			ibinfo.type = RG_BUFFER_TYPE_INDEX;
			ibinfo.stride = sizeof(Uint16);
			ibinfo.length = sizeof(inds);
			ibinfo.initialData = inds;
			indexbuffer = ctx->CreateBuffer(dev, &ibinfo);

			RShaderCreateInfo sinfo = {};
			sinfo.isCompiled = true;
			sinfo.type = RG_SHADER_TYPE_VERTEX;
			sinfo.name = "fwd_test.vs";
			vs = ctx->CreateShader(dev, &sinfo);
			sinfo.type = RG_SHADER_TYPE_PIXEL;
			sinfo.name = "fwd_test.ps";
			ps = ctx->CreateShader(dev, &sinfo);

			RSamplerCreateInfo samplerinfo = {};
			samplerinfo.addressModeU = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeV = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeW = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.filterMode   = RG_SAMPLER_FILTER_LINEAR;
			samplerinfo.maxAnisotropy = 1;
			sampler = ctx->CreateSampler(dev, &samplerinfo);

			CreateFramebuffer(size);
		}

		void DestroyRLighting() {
			RenderBackend* ctx = GetRenderContext();
			DestroyFramebuffer();

			ctx->DestroyShader(vs);
			ctx->DestroyShader(ps);

			ctx->DestroyBuffer(vertexbuffer);
			ctx->DestroyBuffer(indexbuffer);
			ctx->DestroySampler(sampler);
			ctx->DestroyCommandBuffer(cmdbuffer);
		}

		void ResizeRLighting(ivec2* size) {
			DestroyFramebuffer();
			CreateFramebuffer(size);
		}

		void* GetRLightingOutputSet() { return set; }

		void DoRLighting() {
			RenderBackend* ctx = GetRenderContext();

			ctx->ResetCommandBuffer(cmdbuffer);
			ctx->BeginCommandBuffer(cmdbuffer);

			ctx->CmdUseImage(cmdbuffer, rtarget, RG_IMAGE_USAGE_COLOR_ATTACHMENT);

			ctx->CmdUseImage(cmdbuffer, GetGBufferImage(0), RG_IMAGE_USAGE_SHADER_READ_ONLY);
			ctx->CmdUseImage(cmdbuffer, GetGBufferImage(1), RG_IMAGE_USAGE_SHADER_READ_ONLY);
			ctx->CmdUseImage(cmdbuffer, GetGBufferImage(2), RG_IMAGE_USAGE_SHADER_READ_ONLY);


			RRenderpassClearInfo clear = {};
			clear.color[0] = {0.0f, 0.0f, 0.0f, 1.0f};
			RRenderpassBeginInfo rpinfo = {};
			rpinfo.clearinfo = &clear;
			rpinfo.framebuffer = framebuffer;
			rpinfo.renderpass  = renderpass;

			ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);
			ctx->CmdBindPipeline(cmdbuffer, pipeline);
			ctx->CmdBindVertexBuffer(cmdbuffer, vertexbuffer, 0, sizeof(Float32) * 2);
			ctx->CmdBindIndexBuffer(cmdbuffer, indexbuffer, RG_INDEX_U16);

			RBindDescriptorSetsInfo info = {};
			RDescriptorSet* gbufferset = GetGBufferOutputSet();
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

	}
}