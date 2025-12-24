#include <rshared.h>
#include "rendertypesvk.h"
#include <filesystem.h>

RRenderpass* R_CreateRenderpass(RRenderDevice* dev, RRenderpassCreateInfo* info) {
	RRenderpass* rp = (RRenderpass*)dev->allocator->Allocate(sizeof(RRenderpass));
	rp->depthEnabled = false;
	rp->dev = dev;
	rp->rt_count = info->rt_count;

	VkAttachmentDescription attachments[8] = {};
	VkAttachmentReference colorAttachmentRef[8] = {};

	for (Uint32 i = 0; i < info->rt_count; i++) {
		attachments[i].flags          = 0;
		attachments[i].format         = GetImageFormat(info->rt_formats[i]);
		attachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[i].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		colorAttachmentRef[i].attachment = i;
		colorAttachmentRef[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	if (info->use_depth) {
		attachments[info->rt_count].flags          = 0;
		attachments[info->rt_count].format         = GetImageFormat(RG_FORMAT_D32);
		attachments[info->rt_count].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachments[info->rt_count].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[info->rt_count].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[info->rt_count].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[info->rt_count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[info->rt_count].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[info->rt_count].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		colorAttachmentRef[info->rt_count].attachment = info->rt_count;
		colorAttachmentRef[info->rt_count].layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

		rp->depthEnabled = true;
	}
	
	VkAttachmentReference dsAttachmentRef = {};
	dsAttachmentRef.attachment = info->rt_count;
	dsAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = info->rt_count;
	subpass.pColorAttachments    = colorAttachmentRef;
	if (info->use_depth) {
		subpass.pDepthStencilAttachment = &dsAttachmentRef;
	}

	VkSubpassDependency dependency = {};
	dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass    = 0;
	dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo rpinfo = {};
	rpinfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpinfo.attachmentCount = info->rt_count;
	if (info->use_depth) {
		rpinfo.attachmentCount++;
	}
	rpinfo.pAttachments    = attachments;
	rpinfo.subpassCount    = 1;
	rpinfo.pSubpasses      = &subpass;
	rpinfo.dependencyCount = 1;
	rpinfo.pDependencies   = &dependency;

	VkResult r = vkCreateRenderPass(dev->vkdev, &rpinfo, dev->vkalloc, &rp->renderpass);
	if (r != VK_SUCCESS) {
		rgLogError(RG_LOG_RENDER, "VK renderpass creation error");
		dev->allocator->Deallocate(rp);
		return NULL;
	}

	return rp;
}

void R_DestroyRenderpass(RRenderpass* rp) {
	RRenderDevice* dev = rp->dev;
	vkDestroyRenderPass(dev->vkdev, rp->renderpass, dev->vkalloc);
	dev->allocator->Deallocate(rp);
}

static void MakePipelineLayout(RRenderDevice* dev, RPipelineCreateInfo* info, RPipeline* pl) {

	pl->bindings = info->layout->binding_count;

	for (Uint32 i = 0; i < info->layout->binding_count; i++) {
		VkDescriptorSetLayoutBinding binding = {};

		binding.binding         = 0;// info->layout->bindings[i].binding;
		binding.descriptorCount = 1;
		binding.descriptorType  = GetDescriptorType(info->layout->bindings[i].type);
		binding.stageFlags      = GetShaderStage(info->layout->bindings[i].stage);

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings    = &binding;
		vkCreateDescriptorSetLayout(dev->vkdev, &layoutInfo, dev->vkalloc, &pl->dslayout[i]);
	}
	//info->layout->binding_count
	
	VkPushConstantRange graphicsPushConstants[2] = {};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	graphicsPushConstants[0].offset = 0;
	graphicsPushConstants[0].size   = 128; // 128 bytes max
	graphicsPushConstants[1].offset = 128;
	graphicsPushConstants[1].size   = 128; // 128 bytes max
	if (info->type == RG_PIPELINE_TYPE_GRAPHICS) {
		graphicsPushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		graphicsPushConstants[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pipelineLayoutInfo.pushConstantRangeCount = 2;
	} else if (info->type == RG_PIPELINE_TYPE_COMPUTE) {
		graphicsPushConstants[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
	}
	
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount      = info->layout->binding_count;
	pipelineLayoutInfo.pSetLayouts         = pl->dslayout;
	pipelineLayoutInfo.pPushConstantRanges = graphicsPushConstants;

	vkCreatePipelineLayout(dev->vkdev, &pipelineLayoutInfo, dev->vkalloc, &pl->layout);

}

static void MakeComputePipeline(RRenderDevice* dev, RPipelineCreateInfo* info, RPipeline* pl) {

	VkPipelineShaderStageCreateInfo shaderstage = {};
	shaderstage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderstage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderstage.module = info->compute_shader->shader;
	shaderstage.pName  = "main";

	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage  = shaderstage;
	pipelineInfo.layout = pl->layout;
	pipelineInfo.basePipelineHandle = NULL;
	pipelineInfo.basePipelineIndex  = -1;

	VkResult r = vkCreateComputePipelines(dev->vkdev, NULL, 1, &pipelineInfo, dev->vkalloc, &pl->pipeline);

	if (r != VK_SUCCESS) {
		rgLogError(RG_LOG_RENDER, "VK compute pipeline creation error");
	}
}

static void MakeGraphicsPipeline(RRenderDevice* dev, RPipelineCreateInfo* info, RPipeline* pl) {

	VkPipelineShaderStageCreateInfo stages[3] = {}; // 0 - vertex, 1 - pixel, 2 - geometry (if needed)
	stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = info->vertex_shader->shader;
	stages[0].pName  = "main";
	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = info->pixel_shader->shader;
	stages[1].pName  = "main";
	if (info->geometry_shader) {
		stages[2].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[2].stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
		stages[2].module = info->geometry_shader->shader;
		stages[2].pName  = "main";
	}

	VkVertexInputBindingDescription inputbinding = {};
	inputbinding.binding   = 0;
	inputbinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription inputdesc[16] = {};
	Uint32 offset = 0;
	for (Uint32 i = 0; i < info->inputCount; i++) {
		RPipelineInputDescription* input = &info->descriptions[i];
		Uint32 size = GetImageFormatSize(input->format);
		inputbinding.stride += size;
		inputdesc[i].binding  = input->inputSlot;
		inputdesc[i].location = i;
		inputdesc[i].format   = GetImageFormat(input->format);
		inputdesc[i].offset   = offset;
		offset += size;
	}
	inputbinding.stride = offset;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount   = 1;
	vertexInputInfo.pVertexBindingDescriptions      = &inputbinding;
	vertexInputInfo.vertexAttributeDescriptionCount = info->inputCount;
	vertexInputInfo.pVertexAttributeDescriptions    = inputdesc;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount  = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	switch (info->fillmode) {
		case RG_RENDERPASS_FILLMODE_SOLID: { rasterizer.polygonMode = VK_POLYGON_MODE_FILL; break; }
		case RG_RENDERPASS_FILLMODE_WIREFRAME: { rasterizer.polygonMode = VK_POLYGON_MODE_LINE; break; }
		default: { rasterizer.polygonMode = VK_POLYGON_MODE_FILL; break; }
	}
	switch (info->cullmode) {
		case RG_RENDERPASS_CULLMODE_BACK: { rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; break; }
		case RG_RENDERPASS_CULLMODE_FRONT: { rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; break; }
		case RG_RENDERPASS_CULLMODE_NONE: { rasterizer.cullMode = VK_CULL_MODE_NONE; break; }
		default: { rasterizer.cullMode = VK_CULL_MODE_NONE; break; }
	}
	rasterizer.frontFace        = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth        = 1.0f;
	rasterizer.depthBiasEnable  = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable  = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable      = VK_FALSE;
	colorBlending.logicOp           = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount   = 1;
	colorBlending.pAttachments      = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable  = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	if (info->geometry_shader) {
		pipelineInfo.stageCount++;
	}
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	if (info->renderpass->depthEnabled) {
		pipelineInfo.pDepthStencilState = &depthStencil;
	}
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = pl->layout;
	pipelineInfo.renderPass          = info->renderpass->renderpass;
	pipelineInfo.subpass             = 0;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex   = -1;

	VkResult r = vkCreateGraphicsPipelines(dev->vkdev, NULL, 1, &pipelineInfo, dev->vkalloc, &pl->pipeline);

	if (r != VK_SUCCESS) {
		rgLogError(RG_LOG_RENDER, "VK graphics pipeline creation error");
	}
}

RPipeline* R_CreatePipeline(RRenderDevice* dev, RPipelineCreateInfo* info) {
	RPipeline* pl = (RPipeline*)dev->allocator->Allocate(sizeof(RPipeline));
	pl->dev = dev;

	MakePipelineLayout(dev, info, pl);
	if (info->type == RG_PIPELINE_TYPE_COMPUTE) {
		pl->type = VK_PIPELINE_BIND_POINT_COMPUTE;
		MakeComputePipeline(dev, info, pl);
	} else {
		pl->type = VK_PIPELINE_BIND_POINT_GRAPHICS;
		MakeGraphicsPipeline(dev, info, pl);
	}

	return pl;
}

void R_DestroyPipeline(RPipeline* pl) {
	RRenderDevice* dev = pl->dev;
	vkDestroyPipeline(dev->vkdev, pl->pipeline, dev->vkalloc);
	vkDestroyPipelineLayout(dev->vkdev, pl->layout, dev->vkalloc);
	for (Uint32 i = 0; i < pl->bindings; i++) {
		vkDestroyDescriptorSetLayout(dev->vkdev, pl->dslayout[i], dev->vkalloc);
	}
	dev->allocator->Deallocate(pl);
}

RShader* R_CreateShader(RRenderDevice* dev, RShaderCreateInfo* info) {
	RShader* shader = (RShader*)dev->allocator->Allocate(sizeof(RShader));
	shader->dev = dev;

	if (!info->isCompiled) {
		rgLogError(RG_LOG_RENDER, "Shader %s MUST be compiled!", info->name);
		return NULL;
	}

	char path[256];
	char file[256];
	Engine::GetPath(path, 256, RG_PATH_SYSTEM, R_RENDERER_SHORTNAME);
	SDL_snprintf(file, 256, "%s/%s", path, info->name);

	Resource* v_res = Engine::GetResource(file);

	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = v_res->length;
	shaderInfo.pCode    = (Uint32*)v_res->data;
	vkCreateShaderModule(dev->vkdev, &shaderInfo, dev->vkalloc, &shader->shader);

	Engine::FreeResource(v_res);

	return shader;
}

void R_DestroyShader(RShader* shader) {
	RRenderDevice* dev = shader->dev;
	vkDestroyShaderModule(dev->vkdev, shader->shader, dev->vkalloc);
	dev->allocator->Deallocate(shader);
}