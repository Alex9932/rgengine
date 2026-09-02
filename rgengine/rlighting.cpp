#include "rlighting.h"

#include "render.h"
#include "rgbuffer.h"

#include "rgvector.h"
#include "queue.h"

#define RG_RENDER_MAX_LIGHTS 1024

namespace Engine {
	namespace Render {

		static RShader* vs_accum;
		static RShader* ps_accum_global;
		static RShader* ps_accum_point;

		static RSampler* sampler;

		static RPipeline* pipeline_point;
		static RPipeline* pipeline_global;

		static RCommandBuffer* cmdbuffer;

		static RImage*       rtarget = NULL;
		static RFramebuffer* framebuffer = NULL;
		static RRenderpass*  renderpass = NULL;

		static Float32 sphere_verts[] = {
			 0.000000f, -1.000000f,  0.000000f,
			 0.723607f, -0.447220f,  0.525725f,
			-0.276388f, -0.447220f,  0.850649f,
			-0.894426f, -0.447216f,  0.000000f,
			-0.276388f, -0.447220f, -0.850649f,
			 0.723607f, -0.447220f, -0.525725f,
			 0.276388f,  0.447220f,  0.850649f,
			-0.723607f,  0.447220f,  0.525725f,
			-0.723607f,  0.447220f, -0.525725f,
			 0.276388f,  0.447220f, -0.850649f,
			 0.894426f,  0.447216f,  0.000000f,
			 0.000000f,  1.000000f,  0.000000f,
			-0.162456f, -0.850654f,  0.499995f,
			 0.425323f, -0.850654f,  0.309011f,
			 0.262869f, -0.525738f,  0.809012f,
			 0.850648f, -0.525736f,  0.000000f,
			 0.425323f, -0.850654f, -0.309011f,
			-0.525730f, -0.850652f,  0.000000f,
			-0.688189f, -0.525736f,  0.499997f,
			-0.162456f, -0.850654f, -0.499995f,
			-0.688189f, -0.525736f, -0.499997f,
			 0.262869f, -0.525738f, -0.809012f,
			 0.951058f,  0.000000f,  0.309013f,
			 0.951058f,  0.000000f, -0.309013f,
			 0.000000f,  0.000000f,  1.000000f,
			 0.587786f,  0.000000f,  0.809017f,
			-0.951058f,  0.000000f,  0.309013f,
			-0.587786f,  0.000000f,  0.809017f,
			-0.587786f,  0.000000f, -0.809017f,
			-0.951058f,  0.000000f, -0.309013f,
			 0.587786f,  0.000000f, -0.809017f,
			 0.000000f,  0.000000f, -1.000000f,
			 0.688189f,  0.525736f,  0.499997f,
			-0.262869f,  0.525738f,  0.809012f,
			-0.850648f,  0.525736f,  0.000000f,
			-0.262869f,  0.525738f, -0.809012f,
			 0.688189f,  0.525736f, -0.499997f,
			 0.162456f,  0.850654f,  0.499995f,
			 0.525730f,  0.850652f,  0.000000f,
			-0.425323f,  0.850654f,  0.309011f,
			-0.425323f,  0.850654f, -0.309011f,
			 0.162456f,  0.850654f, -0.499995f
		};
		static Uint16 sphere_inds[] = {
			0, 13, 12,
			1, 13, 15,
			0, 12, 17,
			0, 17, 19,
			0, 19, 16,
			1, 15, 22,
			2, 14, 24,
			3, 18, 26,
			4, 20, 28,
			5, 21, 30,
			1, 22, 25,
			2, 24, 27,
			3, 26, 29,
			4, 28, 31,
			5, 30, 23,
			6, 32, 37,
			7, 33, 39,
			8, 34, 40,
			9, 35, 41,
			10, 36, 38,
			38, 41, 11,
			38, 36, 41,
			36, 9, 41,
			41, 40, 11,
			41, 35, 40,
			35, 8, 40,
			40, 39, 11,
			40, 34, 39,
			34, 7, 39,
			39, 37, 11,
			39, 33, 37,
			33, 6, 37,
			37, 38, 11,
			37, 32, 38,
			32, 10, 38,
			23, 36, 10,
			23, 30, 36,
			30, 9, 36,
			31, 35, 9,
			31, 28, 35,
			28, 8, 35,
			29, 34, 8,
			29, 26, 34,
			26, 7, 34,
			27, 33, 7,
			27, 24, 33,
			24, 6, 33,
			25, 32, 6,
			25, 22, 32,
			22, 10, 32,
			30, 31, 9,
			30, 21, 31,
			21, 4, 31,
			28, 29, 8,
			28, 20, 29,
			20, 3, 29,
			26, 27, 7,
			26, 18, 27,
			18, 2, 27,
			24, 25, 6,
			24, 14, 25,
			14, 1, 25,
			22, 23, 10,
			22, 15, 23,
			15, 5, 23,
			16, 21, 5,
			16, 19, 21,
			19, 4, 21,
			19, 20, 4,
			19, 17, 20,
			17, 3, 20,
			17, 18, 3,
			17, 12, 18,
			12, 2, 18,
			15, 16, 5,
			15, 13, 16,
			13, 0, 16,
			12, 14, 2,
			12, 13, 14,
			13, 1, 14
		};

		static float quad_verts[] = { -1, -1, 0, 1, -1, 0, 1, 1, 0, -1, 1, 0 };
		static Uint16 quad_inds[] = { 0, 3, 2, 2, 1, 0 };

		static RBuffer* quad_vb;
		static RBuffer* quad_ib;

		static RBuffer* sphere_vb;
		static RBuffer* sphere_ib;

		static Queue* light_queue;

		static RDescriptorSet* rt_set;

		static void CreateFramebuffer(ivec2* size) {
			RenderBackend* ctx = GetRenderContext();
			RRenderDevice* dev = GetRenderDevice();

			RImageCreateInfo cainfo = {}; // Color attachments
			cainfo.format = RG_FORMAT_R16G16B16A16_FLOAT; // Use 32-bit floats?
			cainfo.width  = size->x;
			cainfo.height = size->y;
			rtarget = ctx->CreateImage(dev, &cainfo);

			RDescriptorSetBinding dsBinding = {};
			dsBinding.binding = 0;
			dsBinding.image = rtarget;
			dsBinding.stage = RG_SHADER_TYPE_PIXEL;
			dsBinding.type = RG_DESCRIPTOR_TYPE_IMAGE;

			RDescriptorSetCreateInfo setInfo = {};
			setInfo.binding_count = 1;
			setInfo.bindings = &dsBinding;
			rt_set = ctx->CreateDescriptorSet(dev, &setInfo);

			RRenderpassCreateInfo rpinfo = {};
			rpinfo.rt_count = 1;
			rpinfo.use_depth = false;
			rpinfo.viewport.x = 0;
			rpinfo.viewport.y = 0;
			rpinfo.viewport.width = size->x;
			rpinfo.viewport.height = size->y;
			rpinfo.rt_formats[0] = RG_FORMAT_R16G16B16A16_FLOAT;
			renderpass = ctx->CreateRenderpass(dev, &rpinfo);

			RFramebufferCreateInfo fbinfo = {};
			fbinfo.width    = size->x;
			fbinfo.height   = size->y;
			fbinfo.rt_count = 1;
			fbinfo.rts[0]   = rtarget;
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
			layout.bindings[2].set = 1;
			layout.bindings[2].binding = 1;
			layout.bindings[2].stage = RG_SHADER_TYPE_PIXEL;
			layout.bindings[2].type = RG_DESCRIPTOR_TYPE_IMAGE;
			layout.bindings[3].set = 1;
			layout.bindings[3].binding = 2;
			layout.bindings[3].stage = RG_SHADER_TYPE_PIXEL;
			layout.bindings[3].type = RG_DESCRIPTOR_TYPE_IMAGE;
			layout.bindings[4].set = 1;
			layout.bindings[4].binding = 3;
			layout.bindings[4].stage = RG_SHADER_TYPE_PIXEL;
			layout.bindings[4].type = RG_DESCRIPTOR_TYPE_IMAGE;
			layout.binding_count = 5;

			RPipelineCreateInfo plinfo = {};
			plinfo.type = RG_PIPELINE_TYPE_GRAPHICS;
			plinfo.cullmode = RG_RENDERPASS_CULLMODE_FRONT;
			plinfo.fillmode = RG_RENDERPASS_FILLMODE_SOLID;
			plinfo.inputCount = 1;
			plinfo.vertex_shader = vs_accum;
			plinfo.pixel_shader = ps_accum_global;
			plinfo.renderpass = renderpass;
			plinfo.descriptions = &description;
			plinfo.layout = &layout;

			plinfo.blendstates[0].blendEnable = 1;
			plinfo.blendstates[0].srcColorFactor = RG_BLEND_FACTOR_ONE;
			plinfo.blendstates[0].dstColorFactor = RG_BLEND_FACTOR_ONE;
			plinfo.blendstates[0].colorBlendOp   = RG_BLEND_OP_ADD;
			plinfo.blendstates[0].srcAlphaFactor = RG_BLEND_FACTOR_ONE;
			plinfo.blendstates[0].dstAlphaFactor = RG_BLEND_FACTOR_ONE;
			plinfo.blendstates[0].alphaBlendOp   = RG_BLEND_OP_ADD;

			pipeline_global = ctx->CreatePipeline(dev, &plinfo);

			// Just change pixel shader for point light pipeline
			plinfo.pixel_shader = ps_accum_point;
			pipeline_point  = ctx->CreatePipeline(dev, &plinfo);
		}

		static void DestroyFramebuffer() {
			RenderBackend* ctx = GetRenderContext();

			ctx->DestroyDescriptorSet(rt_set);
			ctx->DestroyPipeline(pipeline_global);
			ctx->DestroyPipeline(pipeline_point);
			ctx->DestroyFramebuffer(framebuffer);
			ctx->DestroyRenderpass(renderpass);
			ctx->DestroyImage(rtarget);
		}

		static void LoadShaders() {
			RenderBackend* ctx = GetRenderContext();
			RRenderDevice* dev = GetRenderDevice();

			RShaderCreateInfo sinfo = {};
			sinfo.type = RG_SHADER_TYPE_VERTEX;
			sinfo.name = "accum.vs";
			vs_accum = ctx->CreateShader(dev, &sinfo);
			sinfo.type = RG_SHADER_TYPE_PIXEL;
			sinfo.name = "accum_point.ps";
			ps_accum_point = ctx->CreateShader(dev, &sinfo);
			sinfo.type = RG_SHADER_TYPE_PIXEL;
			sinfo.name = "accum_global.ps";
			ps_accum_global = ctx->CreateShader(dev, &sinfo);
		}

		static void DestroyShaders() {
			RenderBackend* ctx = GetRenderContext();
			ctx->DestroyShader(vs_accum);
			ctx->DestroyShader(ps_accum_point);
			ctx->DestroyShader(ps_accum_global);
		}

		void InitRLighting(ivec2* size) {
			RenderBackend* ctx = GetRenderContext();
			RRenderDevice* dev = GetRenderDevice();

			light_queue = RG_NEW(Queue)(RG_RENDER_MAX_LIGHTS);

			RCommandBufferCreateInfo cmdbuffinfo = {};
			cmdbuffinfo.maxcmds = 256;
			cmdbuffer = ctx->CreateCommandBuffer(dev, &cmdbuffinfo);

			RBufferCreateInfo vbinfo = {};
			vbinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
			vbinfo.usage = RG_BUFFER_USAGE_DEFAULT;
			vbinfo.type = RG_BUFFER_TYPE_VERTEX;
			vbinfo.stride = sizeof(Float32) * 3;
			vbinfo.length = sizeof(quad_verts);
			vbinfo.initialData = quad_verts;
			quad_vb = ctx->CreateBuffer(dev, &vbinfo);

			RBufferCreateInfo ibinfo = {};
			ibinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
			ibinfo.usage = RG_BUFFER_USAGE_DEFAULT;
			ibinfo.type = RG_BUFFER_TYPE_INDEX;
			ibinfo.stride = sizeof(Uint16);
			ibinfo.length = sizeof(quad_inds);
			ibinfo.initialData = quad_inds;
			quad_ib = ctx->CreateBuffer(dev, &ibinfo);

			vbinfo = {};
			vbinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
			vbinfo.usage = RG_BUFFER_USAGE_DEFAULT;
			vbinfo.type = RG_BUFFER_TYPE_VERTEX;
			vbinfo.stride = sizeof(Float32) * 3;
			vbinfo.length = sizeof(sphere_verts);
			vbinfo.initialData = sphere_verts;
			sphere_vb = ctx->CreateBuffer(dev, &vbinfo);

			ibinfo = {};
			ibinfo.access = RG_BUFFER_ACCESS_GPU_ONLY;
			ibinfo.usage = RG_BUFFER_USAGE_DEFAULT;
			ibinfo.type = RG_BUFFER_TYPE_INDEX;
			ibinfo.stride = sizeof(Uint16);
			ibinfo.length = sizeof(sphere_inds);
			ibinfo.initialData = sphere_inds;
			sphere_ib = ctx->CreateBuffer(dev, &ibinfo);

			RSamplerCreateInfo samplerinfo = {};
			samplerinfo.addressModeU = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeV = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeW = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.filterMode   = RG_SAMPLER_FILTER_LINEAR;
			samplerinfo.maxAnisotropy = 1;
			sampler = ctx->CreateSampler(dev, &samplerinfo);

			LoadShaders();
			CreateFramebuffer(size);
		}

		void DestroyRLighting() {
			RenderBackend* ctx = GetRenderContext();
			DestroyFramebuffer();
			DestroyShaders();

			ctx->DestroyBuffer(quad_vb);
			ctx->DestroyBuffer(quad_ib);
			ctx->DestroyBuffer(sphere_vb);
			ctx->DestroyBuffer(sphere_ib);
			ctx->DestroySampler(sampler);
			ctx->DestroyCommandBuffer(cmdbuffer);

			RG_DELETE(Queue, light_queue);
		}

		void ResizeRLighting(ivec2* size) {
			DestroyFramebuffer();
			CreateFramebuffer(size);
		}

		void ReloadRLighting(ivec2* size) {
			DestroyShaders();
			LoadShaders();
			ResizeRLighting(size);
		}

		static void CalculateLight(vec4* dir, R3D_GlobalLightDescrition* glight) {
			//if (!dir || !glight) return;
			Float32 time = glight->time;
			Float32 theta = time;
			vec3 d = {};
			d.x = SDL_cosf(theta);
			d.y = SDL_sinf(theta);
			d.z = 0.25f;
			vec3 nd = d.normalize();
			dir->x = nd.x;
			dir->y = nd.y;
			dir->z = nd.z;
		}

		void PushSourceRLighting(R3D_LightSource* light) {
			light_queue->Push(light);
		}

		void DoRLighting() {
			RenderBackend* ctx = GetRenderContext();

			ctx->ResetCommandBuffer(cmdbuffer);
			ctx->BeginCommandBuffer(cmdbuffer);

			ctx->CmdUseImage(cmdbuffer, rtarget, RG_IMAGE_USAGE_COLOR_ATTACHMENT);

			ctx->CmdUseImage(cmdbuffer, GetGBufferImage(0), RG_IMAGE_USAGE_SHADER_READ_ONLY);
			ctx->CmdUseImage(cmdbuffer, GetGBufferImage(1), RG_IMAGE_USAGE_SHADER_READ_ONLY);
			ctx->CmdUseImage(cmdbuffer, GetGBufferImage(2), RG_IMAGE_USAGE_SHADER_READ_ONLY);
			ctx->CmdUseImage(cmdbuffer, GetGBufferDepth(),  RG_IMAGE_USAGE_SHADER_READ_ONLY);


			RRenderpassClearInfo clear = {};
			clear.color[0] = {0.0f, 0.0f, 0.0f, 1.0f};
			RRenderpassBeginInfo rpinfo = {};
			rpinfo.clearinfo = &clear;
			rpinfo.framebuffer = framebuffer;
			rpinfo.renderpass  = renderpass;

			ctx->CmdBeginRenderpass(cmdbuffer, &rpinfo);

			ctx->CmdBindPipeline(cmdbuffer, pipeline_point);
			ctx->CmdBindVertexBuffer(cmdbuffer, sphere_vb, 0, sizeof(Float32) * 3);
			ctx->CmdBindIndexBuffer(cmdbuffer, sphere_ib, RG_INDEX_U16);

			// Bind descriptor sets
			RBindDescriptorSetsInfo info = {};
			RDescriptorSet* gbufferset = GetGBufferOutputSet();
			info.sets = &gbufferset;
			info.startslot = 1;
			info.count = 1;

			ctx->CmdBindDescriptorSets(cmdbuffer, &info);
			ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);

			mat4 mvp;
			R3D_CameraInfo* camera = GetCameraInfo();
			mat4 vp = camera->projection * camera->view;
			mat4 model = MAT4_IDENTITY(); // TODO: set model matrix for each point light


			void* vp_light = NULL;
			//for (Uint32 i = 0; i < 0; i++) {
			while((vp_light = light_queue->Pop()) != NULL) {
				R3D_LightSource* light = (R3D_LightSource*)vp_light;

				Float32 r = light->intensity + light->intensity * light->intensity; // range based on intensity
				mat4_model(&model, light->position, { 0, 0, 0 }, { r, r, r });

				mvp = vp * model;

				struct pc_data {
					mat4 mat;
					vec4 camera; // w - light type (1 - point, 2 - spot)
					vec4 dir;    // xyz - spotlight direction, alpha - light radius (intensity)
					vec4 color;  // alpha - inner cone
					vec4 pos;    // alpha - outer cone
				} data;

				data.mat    = mvp;
				data.camera = { camera->position.x, camera->position.y, camera->position.z, (Float32)light->type };
				data.dir    = { light->direction.x, light->direction.y, light->direction.z, light->intensity };
				data.color  = { light->color.r, light->color.g, light->color.b, light->innerCone };
				data.pos    = { light->position.x, light->position.y, light->position.z, light->outerCone };

				//ctx->CmdPushConstants(cmdbuffer, &mvp, sizeof(mat4), RG_SHADER_TYPE_VERTEX);
				//ctx->CmdPushConstants(cmdbuffer, &data, sizeof(ps_data), RG_SHADER_TYPE_PIXEL);
				ctx->CmdPushConstants(cmdbuffer, &data, sizeof(pc_data));

				// draw point lights here
				ctx->CmdDrawIndexed(cmdbuffer, sizeof(sphere_inds) / sizeof(Uint16), 0);
			}


			ctx->CmdBindPipeline(cmdbuffer, pipeline_global);
			ctx->CmdBindVertexBuffer(cmdbuffer, quad_vb, 0, sizeof(Float32) * 3);
			ctx->CmdBindIndexBuffer(cmdbuffer, quad_ib, RG_INDEX_U16);

			// Need rebind descriptor sets?
			ctx->CmdBindDescriptorSets(cmdbuffer, &info);
			ctx->CmdBindSampler(cmdbuffer, sampler, 0, RG_SHADER_TYPE_PIXEL);

			mvp = MAT4_IDENTITY();
			struct pc_data {
				mat4 mat;
				vec4 camera;    // w - light type (always 0)
				vec4 dir;       // alpha - light intensity
				vec4 color;     // alpha - ambient
			} data;

			R3D_GlobalLightDescrition* glight = GetGlobalLight();
			CalculateLight(&data.dir, glight);

			data.mat    = mvp;
			data.camera = { camera->position.x, camera->position.y, camera->position.z, (Float32)RG_LIGHT_GLOBAL };
			data.dir.w  = glight->intensity;
			data.color  = { glight->color.r, glight->color.g, glight->color.b, glight->ambient };

			//ctx->CmdPushConstants(cmdbuffer, &mvp, sizeof(mat4), RG_SHADER_TYPE_VERTEX);
			//ctx->CmdPushConstants(cmdbuffer, &data, sizeof(ps_data), RG_SHADER_TYPE_PIXEL);
			ctx->CmdPushConstants(cmdbuffer, &data, sizeof(pc_data));

			ctx->CmdDrawIndexed(cmdbuffer, 6, 0);


			ctx->CmdEndRenderpass(cmdbuffer);
			ctx->EndCommandBuffer(cmdbuffer);

			RCommandBufferSubmitInfo submitinfo = {};
			submitinfo.buffer = cmdbuffer;
			ctx->SubmitCommandBuffer(&submitinfo);
			
		}

		RImage* GetRLightingOutput() {
			return rtarget;
		}

		RDescriptorSet* GetRLightingOutputSet() {
			return rt_set;
		}

	}
}