#ifndef _RENDERTYPESVK_H
#define _RENDERTYPESVK_H

#include <rendertypes.h>
#include <vulkan/vulkan.h>
#include <allocator.h>
#include <vma/vk_mem_alloc.h>

#define R_RENDERER_NAME      "Vulkan"
#define R_RENDERER_SHORTNAME "vk"

#define R_VKRENDER_DEBUG 1

#define R_MAX_COMMANDS_PER_BUFFER 256
#define R_MAX_COMMANDBUFFERS_PER_FRAME 256

#define R_VK_FRAMES_IN_FLIGHT 4 // Max frames in flight

struct RRenderDevice {
	VkInstance         vkctx;
	VkPhysicalDevice   vkpdev;
	VkDevice           vkdev;
	VkQueue            vkqueue;
	VkQueue            vktransferqueue;
	Uint32             vkqueuefamily;
	Uint32             vktransferqueuefamily;

	Uint32             cmdsemaphore;
	VkCommandPool      vkcommandpool;
	VkCommandPool      vktransfercommandpool;
	VkCommandBuffer    vkswapcmdbuffer[R_VK_FRAMES_IN_FLIGHT][2];
	VkSemaphore        cmdbuffsemaphores[R_VK_FRAMES_IN_FLIGHT][R_MAX_COMMANDBUFFERS_PER_FRAME];
	VkFence            vkflightfences[R_VK_FRAMES_IN_FLIGHT];
	
#if R_VKRENDER_DEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
#endif

	VkDescriptorPool   vkdescriptorpool;

	VmaAllocator	   vmaallocator;

	VkSurfaceKHR       vksurface;
	VkSurfaceFormatKHR vkswapchainformat;
	VkExtent2D         vkextent;
	VkSwapchainKHR     vkswapchain;
	VkPresentModeKHR   vkpresentmode;
	Uint32             vkimagescount;
	VkImage            vkswapimages[R_VK_FRAMES_IN_FLIGHT];
	VkImageView        vkimageviews[R_VK_FRAMES_IN_FLIGHT];
	VkFramebuffer      vkframebuffers[R_VK_FRAMES_IN_FLIGHT];
	Uint32             vkpresentqueue;
	Uint32             vkcurrentframe;
	Uint32             vkcurrentimage;
	VkSemaphore        vkeximagesemaphore[R_VK_FRAMES_IN_FLIGHT];
	VkSemaphore        vkpresentsemaphore[R_VK_FRAMES_IN_FLIGHT];

	VkRenderPass       imguirenderpass;

	RSampler*          defaultsampler;

	SDL_Window*        hwnd;
	ivec2              wndsize;
	Bool 			   wndresized;
	Bool               isAnisotropicEnabled;
	Uint16 _offset0;
	Uint32             flags;

	VkAllocationCallbacks* vkalloc;
	Engine::Allocator* allocator;

	Uint64 buffersMemLen;
	Uint64 imageMemLen;

	Uint32 draw_calls;
	Uint32 dispatch_calls;

	char cardName[128];
};

#define RG_BUFFER_TYPE_VK_TSRC 0x8000
#define RG_BUFFER_TYPE_VK_TDST 0x8001

struct RBuffer {
	RRenderDevice* dev;
	VkBuffer       buffer;// [R_VK_FRAMES_IN_FLIGHT] ;
	VmaAllocation  allocation;// [R_VK_FRAMES_IN_FLIGHT] ;
	Uint32         length;
	Uint8          access;
	Uint8          _off0;
	Uint16         _off1;
};

struct RImage {
	RRenderDevice* dev;
	VkImage        image;
	VkImageView    view;
	VmaAllocation  allocation;
	Uint32         length;
	RFormat        format;
	VkImageLayout  layout;
	Uint32         usage;
};

struct RCommandBuffer {
	RRenderDevice*  dev;
	VkCommandBuffer cmdbuffer[R_VK_FRAMES_IN_FLIGHT];
	RPipeline*      pipeline[R_VK_FRAMES_IN_FLIGHT]; // Last binded pipeline
};

struct RDescriptorSet {
	RRenderDevice*  dev;
	VkDescriptorSet set[R_VK_FRAMES_IN_FLIGHT];
	VkDescriptorSetLayout layout;
};

#if 0
struct RResourceView {
	RRenderDevice*  dev;
	RFormat         format;
	Uint16          buffer_type;
	Uint16          _off;
	VkImageView     imageView;
	VkDescriptorSet descSet;
	VkDescriptorSetLayout descLayout;
};
#endif

struct RPipeline {
	RRenderDevice*        dev;
	VkPipelineBindPoint   type;
	VkPipeline            pipeline;
	VkPipelineLayout      layout;
	Uint32                bindings;
	Uint32                stages;
	VkDescriptorSetLayout dslayout[16]; // max 16 attachments to pipeline
};

struct RFramebuffer {
	RRenderDevice* dev;
	Uint16         width;
	Uint16         height;
	Uint32         _offset;
	VkFramebuffer  framebuffer[R_VK_FRAMES_IN_FLIGHT];
};

struct RRenderpass {
	RRenderDevice* dev;
	VkRenderPass   renderpass;
	Bool           depthEnabled;
	Uint8          rt_count;
	Uint16         _off0;
	Uint32         _off1;
};

struct RShader {
	RRenderDevice* dev;
	VkShaderModule shader;
};

struct RSampler {
	RRenderDevice*  dev;
	VkSampler       sampler;
	VkDescriptorSet descSet;
	VkDescriptorSetLayout descLayout;
};

static RG_INLINE VkIndexType GetVkIndexType(IndexType type) {
	switch (type) {
		case RG_INDEX_U32: return VK_INDEX_TYPE_UINT32;
		case RG_INDEX_U16: return VK_INDEX_TYPE_UINT16;
		case RG_INDEX_U8:  return VK_INDEX_TYPE_UINT8;
		default: return VK_INDEX_TYPE_UINT16;
	}
}

static VkFormat GetImageFormat(RFormat format) {
	switch (format) {
	case RG_FORMAT_R8_UNORM:           return VK_FORMAT_R8_UNORM;
	case RG_FORMAT_R8G8B8A8_UNORM:     return VK_FORMAT_R8G8B8A8_UNORM;
	case RG_FORMAT_R32_FLOAT:          return VK_FORMAT_R32_SFLOAT;
	case RG_FORMAT_R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
	case RG_FORMAT_R32G32B32_FLOAT:    return VK_FORMAT_R32G32B32_SFLOAT;
	case RG_FORMAT_R32G32_FLOAT:       return VK_FORMAT_R32G32_SFLOAT;
	case RG_FORMAT_D24S8:              return VK_FORMAT_D24_UNORM_S8_UINT;
	case RG_FORMAT_D32:                return VK_FORMAT_D32_SFLOAT;
	case RG_FORMAT_R16_FLOAT:          return VK_FORMAT_R16_SFLOAT;
	case RG_FORMAT_R16G16_FLOAT:       return VK_FORMAT_R16G16_SFLOAT;
	case RG_FORMAT_R16G16B16_FLOAT:    return VK_FORMAT_R16G16B16_SFLOAT;
	case RG_FORMAT_R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
	default:                           return VK_FORMAT_UNDEFINED;
	}
}

static Uint32 GetImageFormatSize(RFormat format) {
	switch (format) {
	case RG_FORMAT_R8_UNORM:           return 1;
	case RG_FORMAT_R8G8B8A8_UNORM:     return 4;
	case RG_FORMAT_R32_FLOAT:          return 4;
	case RG_FORMAT_R32G32B32A32_FLOAT: return 16;
	case RG_FORMAT_R32G32B32_FLOAT:    return 12;
	case RG_FORMAT_R32G32_FLOAT:       return 8;
	case RG_FORMAT_D24S8:              return 4;
	case RG_FORMAT_D32:                return 4;
	case RG_FORMAT_R16_FLOAT:          return 2;
	case RG_FORMAT_R16G16_FLOAT:       return 4;
	case RG_FORMAT_R16G16B16_FLOAT:    return 6;
	case RG_FORMAT_R16G16B16A16_FLOAT: return 8;
	default:                           return 1;
	}
}

static VkShaderStageFlags GetShaderStage(Uint32 stage) {
	VkShaderStageFlags flags = 0;
	if (RG_CHECK_FLAG(stage, RG_SHADER_TYPE_VERTEX)) {
		flags |= VK_SHADER_STAGE_VERTEX_BIT;
	}
	if (RG_CHECK_FLAG(stage, RG_SHADER_TYPE_PIXEL)) {
		flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	if (RG_CHECK_FLAG(stage, RG_SHADER_TYPE_GEOMETRY)) {
		flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
	}
	if (RG_CHECK_FLAG(stage, RG_SHADER_TYPE_COMPUTE)) {
		flags |= VK_SHADER_STAGE_COMPUTE_BIT;
	}
	return flags;
}

static VkDescriptorType GetDescriptorType(Uint32 type) {
	switch (type) {
	case RG_DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
	case RG_DESCRIPTOR_TYPE_IMAGE:   return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case RG_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case RG_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	default: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

static VkImageLayout GetImageLayout(Uint32 usage) {
	switch (usage) {
	case RG_IMAGE_USAGE_UNDEFINED:        { return VK_IMAGE_LAYOUT_UNDEFINED; }
	case RG_IMAGE_USAGE_COLOR_ATTACHMENT: { return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; }
	case RG_IMAGE_USAGE_DEPTH_ATTACHMENT: { return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL; }
	case RG_IMAGE_USAGE_SHADER_READ_ONLY: { return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }
	case RG_IMAGE_USAGE_TRANSFER_SRC:     { return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; }
	case RG_IMAGE_USAGE_TRANSFER_DST:     { return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; }
	default: { return VK_IMAGE_LAYOUT_GENERAL; }
	}
}

static VkPipelineStageFlagBits GetImagePipelineStage(Uint32 usage) {
	switch (usage) {
	case RG_IMAGE_USAGE_UNDEFINED:        { return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; }
	case RG_IMAGE_USAGE_COLOR_ATTACHMENT: { return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; }
	case RG_IMAGE_USAGE_DEPTH_ATTACHMENT: { return (VkPipelineStageFlagBits)(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT); }
	case RG_IMAGE_USAGE_SHADER_READ_ONLY: { return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; }
	case RG_IMAGE_USAGE_TRANSFER_SRC:     { return VK_PIPELINE_STAGE_TRANSFER_BIT; }
	case RG_IMAGE_USAGE_TRANSFER_DST:     { return VK_PIPELINE_STAGE_TRANSFER_BIT; }
	default: { return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; }
	}
}

static VkAccessFlagBits GetImageAccess(Uint32 usage) {
	switch (usage) {
	case RG_IMAGE_USAGE_UNDEFINED:        { return VK_ACCESS_NONE; }
	case RG_IMAGE_USAGE_COLOR_ATTACHMENT: { return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; }
	case RG_IMAGE_USAGE_DEPTH_ATTACHMENT: { return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; }
	case RG_IMAGE_USAGE_SHADER_READ_ONLY: { return VK_ACCESS_SHADER_READ_BIT; }
	case RG_IMAGE_USAGE_TRANSFER_SRC:     { return VK_ACCESS_TRANSFER_READ_BIT; }
	case RG_IMAGE_USAGE_TRANSFER_DST:     { return VK_ACCESS_TRANSFER_WRITE_BIT; }
	default: { return VK_ACCESS_NONE; }
	}
}

static VkBlendOp GetBlendOp(Uint32 op) {
	switch (op) {
	case RG_BLEND_OP_ADD:     return VK_BLEND_OP_ADD;
	case RG_BLEND_OP_SUB:     return VK_BLEND_OP_SUBTRACT;
	case RG_BLEND_OP_REV_SUB: return VK_BLEND_OP_REVERSE_SUBTRACT;
	case RG_BLEND_OP_MIN:     return VK_BLEND_OP_MIN;
	case RG_BLEND_OP_MAX:     return VK_BLEND_OP_MAX;
	default:                  return VK_BLEND_OP_ADD;
	}
}

static VkBlendFactor GetBlendFactor(Uint32 factor) {
	switch (factor) {
	case RG_BLEND_FACTOR_ZERO:                return VK_BLEND_FACTOR_ZERO;
	case RG_BLEND_FACTOR_ONE:                 return VK_BLEND_FACTOR_ONE;
	case RG_BLEND_FACTOR_SRC_COLOR:           return VK_BLEND_FACTOR_SRC_COLOR;
	case RG_BLEND_FACTOR_DST_COLOR:           return VK_BLEND_FACTOR_DST_COLOR;
	case RG_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case RG_BLEND_FACTOR_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case RG_BLEND_FACTOR_SRC_ALPHA:           return VK_BLEND_FACTOR_SRC_ALPHA;
	case RG_BLEND_FACTOR_DST_ALPHA:           return VK_BLEND_FACTOR_DST_ALPHA;
	case RG_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case RG_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	default:                                  return VK_BLEND_FACTOR_ONE;
	}
}

#endif