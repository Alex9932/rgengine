#include "ranimator.h"

#include "render.h"
#include "engine.h"
#include "modelsystem.h"
#include "kinematicsmodel.h"

namespace Engine {
	namespace Render {

        static RShader*        shader    = NULL;
        static RPipeline*      pipeline  = NULL;
        static RCommandBuffer* cmdbuffer = NULL;

        void InitRenderAnimation() {
            RenderBackend* ctx = GetRenderContext();
            RRenderDevice* dev = GetRenderDevice();

            // Shader

            RShaderCreateInfo csinfo = {};
            csinfo.isCompiled = true;
            csinfo.name = "skinning.cs";
            csinfo.type = RG_SHADER_TYPE_COMPUTE;
            shader = ctx->CreateShader(dev, &csinfo);


            // Pipeline

            RPipelineLayoutDescription cslayout = {};
            cslayout.binding_count = 4;

            // Matrices
            cslayout.bindings[0].set = 0;
            cslayout.bindings[0].binding = 0;
            cslayout.bindings[0].stage = RG_SHADER_TYPE_COMPUTE;
            cslayout.bindings[0].type = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            // Input vertices
            cslayout.bindings[1].set = 1;
            cslayout.bindings[1].binding = 0;
            cslayout.bindings[1].stage = RG_SHADER_TYPE_COMPUTE;
            cslayout.bindings[1].type = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            // Input weights
            cslayout.bindings[2].set = 1;
            cslayout.bindings[2].binding = 1;
            cslayout.bindings[2].stage = RG_SHADER_TYPE_COMPUTE;
            cslayout.bindings[2].type = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            // Output vertices
            cslayout.bindings[3].set = 1;
            cslayout.bindings[3].binding = 2;
            cslayout.bindings[3].stage = RG_SHADER_TYPE_COMPUTE;
            cslayout.bindings[3].type = RG_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            RPipelineCreateInfo cplinfo = {};
            cplinfo.type = RG_PIPELINE_TYPE_COMPUTE;
            cplinfo.compute_shader = shader;
            cplinfo.inputCount = 0;
            cplinfo.descriptions = NULL;
            cplinfo.layout = &cslayout;
            pipeline = ctx->CreatePipeline(dev, &cplinfo);

            // Command buffer

            RCommandBufferCreateInfo cmdbuffinfo = {};
            cmdbuffinfo.maxcmds = 128;
            cmdbuffer = ctx->CreateCommandBuffer(dev, &cmdbuffinfo);

        }

        void DestroyRenderAnimation() {
            RenderBackend* ctx = GetRenderContext();
            ctx->DestroyCommandBuffer(cmdbuffer);
            ctx->DestroyPipeline(pipeline);
            ctx->DestroyShader(shader);
        }

        void ReloadRenderAnimation() {
            DestroyRenderAnimation();
            InitRenderAnimation();
        }

		void DoAnimate() {

            RenderBackend* ctx = GetRenderContext();
            ModelSystem* mdlsystem = GetModelSystem();

            ctx->ResetCommandBuffer(cmdbuffer);
            ctx->BeginCommandBuffer(cmdbuffer);

            // Calculate skeleton animations
            ctx->CmdBindPipeline(cmdbuffer, pipeline);
            for (Uint32 i = 0; i < mdlsystem->GetRiggedModelCount(); i++) {
                RiggedModelComponent* com = mdlsystem->GetRiggedModelComponent(i);
                R3D_BoneBuffer* bbuf = com->GeBoneBuffer();
                R3D_RiggedModel* mdl = com->GetHandle();

                RDescriptorSet* sets[2] = { bbuf->set, mdl->set };
                RBindDescriptorSetsInfo dsinfo = {};
                dsinfo.count = 2;
                dsinfo.startslot = 0;
                dsinfo.sets = sets;
                ctx->CmdBindDescriptorSets(cmdbuffer, &dsinfo);
                ctx->CmdDispatch(cmdbuffer, mdl->vCount, 1, 1);
            }

            ctx->EndCommandBuffer(cmdbuffer);

            RCommandBufferSubmitInfo submitinfo = {};
            submitinfo.buffer = cmdbuffer;
            ctx->SubmitCommandBuffer(&submitinfo);

		}

	}
}