#include "rgbufferdraw.h"
#include "rgbuffer.h"

#include "rgmatrix.h"

#include "render.h"
#include "texture.h"
#include "material.h"

namespace Engine {
	namespace Render {

		struct GBufferUniformsVS {
			mat4 proj;
			mat4 view;
		} uniforms_vs;

		static RDescriptorSet* ubufferdesc = NULL;
		static RBuffer*        ubuffer     = NULL;
		static RCommandBuffer* cmdbuffer   = NULL;
		static RSampler*       sampler     = NULL;

		void InitGBufferDraw() {
			RRenderDevice* dev = GetRenderDevice();
			RenderBackend* ctx = GetRenderContext();

			RCommandBufferCreateInfo cmdbuffinfo = {};
			cmdbuffinfo.maxcmds = 128;
			cmdbuffer = ctx->CreateCommandBuffer(dev, &cmdbuffinfo);

			RSamplerCreateInfo samplerinfo = {};
#if 0
			samplerinfo.addressModeU  = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeV  = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
			samplerinfo.addressModeW  = RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE;
#else
			samplerinfo.addressModeU  = RG_SAMPLER_ADDRESSMODE_REPEAT;
			samplerinfo.addressModeV  = RG_SAMPLER_ADDRESSMODE_REPEAT;
			samplerinfo.addressModeW  = RG_SAMPLER_ADDRESSMODE_REPEAT;
#endif
			samplerinfo.filterMode    = RG_SAMPLER_FILTER_ANISOTROPIC;
			samplerinfo.maxAnisotropy = 16;
			sampler = ctx->CreateSampler(dev, &samplerinfo);

			RBufferCreateInfo ubufferInfo = {};
			ubufferInfo.length = sizeof(GBufferUniformsVS);
			ubufferInfo.type   = RG_BUFFER_TYPE_CONSTANT;
			ubufferInfo.usage  = RG_BUFFER_USAGE_DYNAMIC;
			ubufferInfo.access = RG_BUFFER_ACCESS_CPU_WRITE;
			ubufferInfo.stride = 1;
			ubuffer = ctx->CreateBuffer(dev, &ubufferInfo);

			RDescriptorSetBinding binding = {};
			binding.binding = 0;
			binding.type = RG_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			binding.buffer = ubuffer;
			binding.stage = RG_SHADER_TYPE_VERTEX;

			RDescriptorSetCreateInfo dsinfo = {};
			dsinfo.bindings = &binding;
			dsinfo.binding_count = 1;
			ubufferdesc = ctx->CreateDescriptorSet(dev, &dsinfo);
		}

		void DestroyGBufferDraw() {
			RenderBackend* ctx = GetRenderContext();
			ctx->DestroyCommandBuffer(cmdbuffer);
			ctx->DestroySampler(sampler);
			ctx->DestroyDescriptorSet(ubufferdesc);
			ctx->DestroyBuffer(ubuffer);
		}

		void BeginGBufferPass(mat4* proj, mat4* view) {
			RenderBackend* ctx = GetRenderContext();

			uniforms_vs.proj = *proj;
			uniforms_vs.view = *view;

			RUpdateBufferInfo binfo = {};
			binfo.handle = ubuffer;
			binfo.offset = 0;
			binfo.length = sizeof(GBufferUniformsVS);
			binfo.data   = &uniforms_vs;
			ctx->UpdateBuffer(&binfo);

			ctx->ResetCommandBuffer(cmdbuffer);
			ctx->BeginCommandBuffer(cmdbuffer);

			// Draw 3D scene
			RRenderpassClearInfo clearinfo = {};
			clearinfo.color[0] = { 0, 0, 0, 1 };
			clearinfo.depth    = 1.0f;
			clearinfo.stencil  = 0;
			RRenderpassBeginInfo rpbegininfo = {};
			rpbegininfo.framebuffer = GetGBufferFramebuffer();
			rpbegininfo.renderpass  = GetGBufferRenderpass();
			rpbegininfo.clearinfo   = &clearinfo;
			ctx->CmdBeginRenderpass(cmdbuffer, &rpbegininfo);
			ctx->CmdBindPipeline(cmdbuffer, GetGBufferPipeline());

			RBindDescriptorSetsInfo info = {};
			info.sets      = &ubufferdesc;
			info.startslot = 0;
			info.count     = 1;
			ctx->CmdBindDescriptorSets(cmdbuffer, &info);
			ctx->CmdBindSampler(cmdbuffer, sampler, 2, RG_SHADER_TYPE_PIXEL);

		}

		void EndGBufferPass() {
			RenderBackend* ctx = GetRenderContext();

			ctx->CmdEndRenderpass(cmdbuffer);
			ctx->EndCommandBuffer(cmdbuffer);

			RCommandBufferSubmitInfo submitinfo = {};
			submitinfo.buffer = cmdbuffer;
			ctx->SubmitCommandBuffer(&submitinfo);
		}

		void DrawGBufferStatic(R3D_StaticModel* mdl, mat4* transform) {
			RenderBackend* ctx = GetRenderContext();

			R3D_Material* current_mat = NULL;
			Bool useMaterial = true;

			// Bind vertexbuffer
			ctx->CmdBindVertexBuffer(cmdbuffer, mdl->vBuffer, 0, sizeof(R3D_Vertex));
			ctx->CmdBindIndexBuffer(cmdbuffer, mdl->iBuffer, mdl->iType);

			ctx->CmdPushConstants(cmdbuffer, transform, sizeof(mat4), RG_SHADER_TYPE_VERTEX);

			for (Uint32 i = 0; i < mdl->mCount; i++) {
				R3D_MeshInfo* minfo = &mdl->info[i];
				R3D_Material* mat = minfo->material;

				if (!mat->descset) {
					MakeMaterialDescriptorSet(mat);
					continue;
				}

				// Bind material
				if (useMaterial && current_mat != mat) {

					vec4 color = { mat->color.r, mat->color.g, mat->color.b, 1 };

					ctx->CmdPushConstants(cmdbuffer, &color, sizeof(vec4), RG_SHADER_TYPE_PIXEL);

					// Bind textures

					RBindDescriptorSetsInfo info = {};
					info.count = 1;
					info.startslot = 1;
					info.sets = &mat->descset;
					ctx->CmdBindDescriptorSets(cmdbuffer, &info);
				}

				// Draw mesh
				ctx->CmdDrawIndexed(cmdbuffer, minfo->indexCount, minfo->indexOffset);
			}

		}

	}
}