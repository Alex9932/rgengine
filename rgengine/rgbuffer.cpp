#include "rgbuffer.h"
#include "rgbufferdraw.h"

#include "render.h"

namespace Engine {
	namespace Render {

		// Color, normal, position
		static RImage* rtargets[3] = {};
		static RImage* depthbuffer = NULL;

		static RFramebuffer* framebuffer = NULL;
		static RRenderpass* renderpass = NULL;
		static RPipeline* pipeline = NULL;
		static RShader* vs = NULL;
		static RShader* ps = NULL;

		static RDescriptorSet* set = NULL;

		static void CreateBuffers(ivec2* wndSize) {
			RRenderDevice* dev = GetRenderDevice();
			RenderBackend* ctx = GetRenderContext();

			RRect wndrect = {};
			wndrect.x = 0;
			wndrect.y = 0;
			wndrect.width  = wndSize->x;
			wndrect.height = wndSize->y;

			RImageCreateInfo cainfo = {}; // Color attachments
			cainfo.format = RG_FORMAT_R16G16B16A16_FLOAT;
			cainfo.width  = wndSize->x;
			cainfo.height = wndSize->y;
			rtargets[0] = ctx->CreateImage(dev, &cainfo);
			rtargets[1] = ctx->CreateImage(dev, &cainfo);
			rtargets[2] = ctx->CreateImage(dev, &cainfo);

			RDescriptorSetBinding bindings[3] = {};
			bindings[0].binding = 0;
			bindings[0].stage = RG_SHADER_TYPE_PIXEL;
			bindings[0].type = RG_DESCRIPTOR_TYPE_IMAGE;
			bindings[0].image = rtargets[0];
			bindings[1].binding = 1;
			bindings[1].stage = RG_SHADER_TYPE_PIXEL;
			bindings[1].type = RG_DESCRIPTOR_TYPE_IMAGE;
			bindings[1].image = rtargets[1];
			bindings[2].binding = 2;
			bindings[2].stage = RG_SHADER_TYPE_PIXEL;
			bindings[2].type = RG_DESCRIPTOR_TYPE_IMAGE;
			bindings[2].image = rtargets[2];

			RDescriptorSetCreateInfo setinfo = {};
			setinfo.binding_count = 3;
			setinfo.bindings = bindings;
			set = ctx->CreateDescriptorSet(dev, &setinfo);

			RImageCreateInfo dbinfo = {}; // Depth attachment
			dbinfo.format = RG_FORMAT_D32;
			dbinfo.width  = wndSize->x;
			dbinfo.height = wndSize->y;
			depthbuffer = ctx->CreateImage(dev, &dbinfo);

			RRenderpassCreateInfo rp3dinfo = {};
			rp3dinfo.rt_count  = 3;
			rp3dinfo.use_depth = true;
			rp3dinfo.viewport  = wndrect;
			rp3dinfo.rt_formats[0] = RG_FORMAT_R16G16B16A16_FLOAT;
			rp3dinfo.rt_formats[1] = RG_FORMAT_R16G16B16A16_FLOAT;
			rp3dinfo.rt_formats[2] = RG_FORMAT_R16G16B16A16_FLOAT;
			renderpass = ctx->CreateRenderpass(dev, &rp3dinfo);

			RPipelineLayoutDescription layout = {};
			layout.binding_count = 5;
			layout.bindings[0].set     = 0;
			layout.bindings[0].binding = 0;
			layout.bindings[0].stage   = RG_SHADER_TYPE_VERTEX;
			layout.bindings[0].type    = RG_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layout.bindings[1].set     = 1;
			layout.bindings[1].binding = 0;
			layout.bindings[1].stage   = RG_SHADER_TYPE_PIXEL;
			layout.bindings[1].type    = RG_DESCRIPTOR_TYPE_IMAGE;
			layout.bindings[2].set     = 1;
			layout.bindings[2].binding = 1;
			layout.bindings[2].stage   = RG_SHADER_TYPE_PIXEL;
			layout.bindings[2].type    = RG_DESCRIPTOR_TYPE_IMAGE;
			layout.bindings[3].set     = 1;
			layout.bindings[3].binding = 2;
			layout.bindings[3].stage   = RG_SHADER_TYPE_PIXEL;
			layout.bindings[3].type    = RG_DESCRIPTOR_TYPE_IMAGE;
			layout.bindings[4].set     = 2;
			layout.bindings[4].binding = 0;
			layout.bindings[4].stage   = RG_SHADER_TYPE_PIXEL;
			layout.bindings[4].type    = RG_DESCRIPTOR_TYPE_SAMPLER;

			RPipelineInputDescription inputdescriptions[4] = {};
			inputdescriptions[0].format    = RG_FORMAT_R32G32B32_FLOAT;
			inputdescriptions[0].inputSlot = 0;
			inputdescriptions[0].name      = "POSITION";
			inputdescriptions[1].format    = RG_FORMAT_R32G32B32_FLOAT;
			inputdescriptions[1].inputSlot = 0;
			inputdescriptions[1].name      = "NORMAL";
			inputdescriptions[2].format    = RG_FORMAT_R32G32B32_FLOAT;
			inputdescriptions[2].inputSlot = 0;
			inputdescriptions[2].name      = "TANGENT";
			inputdescriptions[3].format    = RG_FORMAT_R32G32_FLOAT;
			inputdescriptions[3].inputSlot = 0;
			inputdescriptions[3].name      = "VPOS";

			RPipelineCreateInfo plinfo = {};
			plinfo.type          = RG_PIPELINE_TYPE_GRAPHICS;
			plinfo.vertex_shader = vs;
			plinfo.pixel_shader  = ps;
			plinfo.inputCount    = 4;
			plinfo.descriptions  = inputdescriptions;
			plinfo.renderpass    = renderpass;
			plinfo.layout        = &layout;
			plinfo.cullmode      = RG_RENDERPASS_CULLMODE_BACK;
			plinfo.fillmode      = RG_RENDERPASS_FILLMODE_SOLID;
			pipeline = ctx->CreatePipeline(dev, &plinfo);

			RFramebufferCreateInfo fbinfo = {};
			fbinfo.width  = wndSize->x;
			fbinfo.height = wndSize->y;
			fbinfo.rt_count = 3;
			fbinfo.rts[0] = rtargets[0];
			fbinfo.rts[1] = rtargets[1];
			fbinfo.rts[2] = rtargets[2];
			fbinfo.dsv    = depthbuffer;
			fbinfo.renderpass = renderpass;
			framebuffer = ctx->CreateFramebuffer(dev, &fbinfo);
		}

		static void FreeBuffers() {
			RenderBackend* ctx = GetRenderContext();
			ctx->DestroyDescriptorSet(set);
			ctx->DestroyFramebuffer(framebuffer);
			ctx->DestroyPipeline(pipeline);
			ctx->DestroyRenderpass(renderpass);
			ctx->DestroyImage(depthbuffer);
			for (Uint32 i = 0; i < 3; i++) {
				ctx->DestroyImage(rtargets[i]);
			}

		}

		void InitGBuffer(ivec2* wndsize) {
			RRenderDevice* dev = GetRenderDevice();
			RenderBackend* ctx = GetRenderContext();

			RShaderCreateInfo vsinfo = {};
			vsinfo.isCompiled = true;
			vsinfo.name = "gbuffer.vs";
			vsinfo.type = RG_SHADER_TYPE_VERTEX;
			vs = ctx->CreateShader(dev, &vsinfo);

			RShaderCreateInfo psinfo = {};
			psinfo.isCompiled = true;
			psinfo.name = "gbuffer.ps";
			psinfo.type = RG_SHADER_TYPE_PIXEL;
			ps = ctx->CreateShader(dev, &psinfo);

			CreateBuffers(wndsize);
			InitGBufferDraw();
		}

		void DestroyGBuffer() {
			RenderBackend* ctx = GetRenderContext();

			DestroyGBufferDraw();
			FreeBuffers();

			ctx->DestroyShader(vs);
			ctx->DestroyShader(ps);
		}

		void ResizeGBuffer(ivec2* wndsize) {
			FreeBuffers();
			CreateBuffers(wndsize);
		}

		RDescriptorSet* GetGBufferOutputSet() { return set; }
		RFramebuffer* GetGBufferFramebuffer() { return framebuffer; }
		RRenderpass* GetGBufferRenderpass() { return renderpass; }
		RPipeline* GetGBufferPipeline() { return pipeline; }

	}
}