#ifndef _RENDERTYPES_H
#define _RENDERTYPES_H

#include "rgtypes.h"
#include "rgvector.h"
#include "rgmatrix.h"
#include "rgmath.h"

#include "imgui/imgui.h"

#include "uuid.h"

enum IndexType {
	RG_INDEX_U32 = 4,
	RG_INDEX_U16 = 2,
	RG_INDEX_U8  = 1  // !!! DO NOT RECOMENDED TO USE !!!
};

enum LightType {
	RG_POINTLIGHT = 0,
	RG_SPOTLIGHT = 1
};

enum TextureType {
	RG_FORMAT_UNKNOWN = 0,

	RG_TEXTURE_U8_R_ONLY  = 1, // 8-bit per channel (R)
	RG_TEXTURE_U8_RGBA    = 2, // 8-bit per channel (RGBA)
	RG_TEXTURE_F32_R_ONLY = 3, // 32-bit float per channel (R)
	RG_TEXTURE_F32_RGBA   = 4, // 32-bit float per channel (RGBA)
	
	RG_FORMAT_R8_UNORM           = 1,
	RG_FORMAT_R8G8B8A8_UNORM     = 2,
	RG_FORMAT_R32_FLOAT          = 3,
	RG_FORMAT_R32G32B32A32_FLOAT = 4,
	RG_FORMAT_R32G32B32_FLOAT    = 5,
	RG_FORMAT_R32G32_FLOAT       = 6,
	RG_FORMAT_D24S8 = 7, // Depth 24-bit + Stencil 8-bit
	RG_FORMAT_D32   = 8, // Depth 32-bit

	RG_FORMAT_R16G16B16A16_FLOAT = 10,
	RG_FORMAT_R16G16B16_FLOAT    = 11,
	RG_FORMAT_R16G16_FLOAT       = 12,
	RG_FORMAT_R16_FLOAT          = 13,
};

#define RFormat TextureType

#define RG_RENDER_FULLSCREEN    1
#define RG_RENDER_USE3D         2
#define RG_RENDER_NOPOSTPROCESS 4
#define RG_RENDER_NOLIGHT       8

// Backend handles

typedef struct RRenderDevice RRenderDevice;
typedef struct RBuffer RBuffer;
typedef struct RImage RImage;
typedef struct RCommandBuffer RCommandBuffer;
typedef struct RDescriptorSet RDescriptorSet;
//typedef struct RResourceView RResourceView;
typedef struct RSampler RSampler;

typedef struct RShader RShader;

typedef struct RPipeline RPipeline;
typedef struct RFramebuffer RFramebuffer;
typedef struct RRenderpass RRenderpass;

////////////

typedef struct RRenderSetupInfo {
	Uint32      flags;
	SDL_Window* hwnd;
} RRenderSetupInfo;

#define RG_SWAPCHAIN_FLAG_RESIZE     0x01

typedef struct RSwapBuffersInfo {
	Uint32 flags;
	ivec2  newsize;
} RSwapBuffersInfo;

// Image

#define RG_IMAGE_LAYOUT_UNDEFINED        0x00
#define RG_IMAGE_LAYOUT_COLOR_ATTACHMENT 0x01
#define RG_IMAGE_LAYOUT_DEPTH_ATTACHMENT 0x02
#define RG_IMAGE_LAYOUT_SHADER_READ_ONLY 0x03
#define RG_IMAGE_LAYOUT_TRANSFER_SRC     0x04
#define RG_IMAGE_LAYOUT_TRANSFER_DST     0x05

typedef struct RImageCreateInfo {
	RFormat format;
	Uint32  width;
	Uint32  height;
	void*   initialData;
} RImageCreateInfo;

// Framebuffer

typedef struct RFramebufferCreateInfo {
	Uint16  width;
	Uint16  height;
	Uint32  rt_count;
	RImage* rts[6];
	RImage* dsv;
	RRenderpass* renderpass;
} RFramebufferCreateInfo;

// Buffer

#define RG_BUFFER_USAGE_DEFAULT    0x0
#define RG_BUFFER_USAGE_DYNAMIC    0x1

#define RG_BUFFER_ACCESS_GPU_ONLY   0x0
#define RG_BUFFER_ACCESS_CPU_WRITE  0x1
#define RG_BUFFER_ACCESS_CPU_READ   0x2

#define RG_BUFFER_TYPE_VERTEX     0x1
#define RG_BUFFER_TYPE_INDEX      0x2
#define RG_BUFFER_TYPE_CONSTANT   0x4
#define RG_BUFFER_TYPE_SHADER_RES 0x8
#define RG_BUFFER_TYPE_UNORDERED  0x10
#define RG_BUFFER_TYPE_STRUCTURED 0x20

typedef struct RBufferCreateInfo {
	Uint32  length; // in bytes
	Uint16  type;
	Uint8   usage;
	Uint8   access;
	Uint32  stride;
	void*   initialData;
} RBufferCreateInfo;

// Cmd buffer

typedef struct RCommandBufferSubmitInfo {
	RCommandBuffer* buffer;
} RCommandBufferSubmitInfo;

typedef struct RCommandBufferCreateInfo {
	Uint32 maxcmds;
} RCommandBufferCreateInfo;

// Resource view
#if 0
#define RG_RESOURCEVIEW_TYPE_RTV 0x1
#define RG_RESOURCEVIEW_TYPE_DSV 0x2
#define RG_RESOURCEVIEW_TYPE_SRV 0x3
#define RG_RESOURCEVIEW_TYPE_UAV 0x4
#define RG_RESOURCEVIEW_TYPE_BBV 0x5 // Swapchain backbuffer

#define RG_RESOURCEVIEW_IMAGE  0x0
#define RG_RESOURCEVIEW_BUFFER 0x1

typedef struct RResourceViewCreateInfo {
	Uint8  type;
	Uint8  stage;
	Uint16 buffer_type;

	union {
		Uint32 var;
		Uint32 elements;
	};

	union {
		RBuffer* dst_buffer;
		RImage*  dst_image;
	};
} RResourceViewCreateInfo;
#endif
// Pipeline

#define RG_PIPELINE_TYPE_GRAPHICS 0x1
#define RG_PIPELINE_TYPE_COMPUTE  0x2


typedef struct RPipelineInputDescription {
	String name;
	Uint32 inputSlot;
	RFormat format;
} RPipelineInputDescription;

#define RG_DESCRIPTOR_TYPE_SAMPLER        0x01
#define RG_DESCRIPTOR_TYPE_IMAGE          0x02
#define RG_DESCRIPTOR_TYPE_UNIFORM_BUFFER 0x03
#define RG_DESCRIPTOR_TYPE_STORAGE_BUFFER 0x04

typedef struct RPipelineLayoutBinding {
	Uint8 binding;
	Uint8 type;
	Uint8 stage;
	union {
		Uint8 _offset;
		Uint8 set; // For future use
	};
} RPipelineLayoutBinding;

typedef struct RPipelineLayoutDescription {
	RPipelineLayoutBinding bindings[16];
	Uint32 binding_count;
} RPipelineLayoutDescription;

typedef struct RPipelineCreateInfo {
	Uint8    type; // Graphics, compute
	Uint8    _off0;
	Uint8    cullmode;
	Uint8    fillmode;
	Uint32   inputCount;

	RShader* vertex_shader;
	RShader* pixel_shader;
	RShader* geometry_shader;

	// Use union for less memory usage
	union {
		void* cs_or_pl_pointer;
		RShader* compute_shader; // Used for create compute pipeline
		RRenderpass* renderpass; // Used for create graphics pipeline
	};

	RPipelineInputDescription* descriptions;
	RPipelineLayoutDescription* layout;
} RPipelineCreateInfo;

// Renderpass

#define RG_RENDERPASS_CULLMODE_NONE        0x0
#define RG_RENDERPASS_CULLMODE_FRONT       0x1
#define RG_RENDERPASS_CULLMODE_BACK        0x2

#define RG_RENDERPASS_FILLMODE_SOLID       0x0
#define RG_RENDERPASS_FILLMODE_WIREFRAME   0x1

typedef struct RRect {
	Float32 x;
	Float32 y;
	Float32 width;
	Float32 height;
} RRect;

typedef struct RRenderpassCreateInfo {
	Uint16  _offset;
	Uint8   rt_count;
	Uint8   use_depth;
	RRect   viewport;
	RFormat rt_formats[6];
	// TODO: add blend, rasterizer, depth-stencil states
} RRenderpassCreateInfo;

typedef struct RRenderpassClearInfo {
	vec4           color[6];
	Float32        depth;
	Uint8          stencil;
} RRenderpassClearInfo;

typedef struct RRenderpassBeginInfo {
	RRenderpass*          renderpass;
	RFramebuffer*         framebuffer;
	RRenderpassClearInfo* clearinfo;
} RRenderpassBeginInfo;

#define RG_SHADER_TYPE_VERTEX    0x01
#define RG_SHADER_TYPE_PIXEL     0x02
#define RG_SHADER_TYPE_GEOMETRY  0x04
#define RG_SHADER_TYPE_COMPUTE   0x20

typedef struct RShaderCreateInfo {
	String name; // Loaded from "platform/&renderername&/"
	Uint8  type; // Vertex, pixel, compute, etc.
	Uint8  isCompiled;
	Uint16 _offset1;
	Uint32 _offset2;
} RShaderCreateInfo;

typedef struct RDescriptorSetBinding {
	Uint8  binding; // Max 16 bindings per set
	Uint8  type; // See RG_DESCRIPTOR_TYPE_
	Uint8  stage;
	Uint8  _offset0;
	Uint32 _offset1;
	union {
		RImage*  image;
		RBuffer* buffer;
	};
} RDescriptorSetBinding;

typedef struct RDescriptorSetCreateInfo {
	RDescriptorSetBinding* bindings;
	Uint16 binding_count;
} RDescriptorSetCreateInfo;

typedef struct RBindDescriptorSetsInfo {
	RDescriptorSet** sets;
	Uint8            startslot; // First bind slot
	Uint8            count;     // Set count
	Uint16           _offset0;
	Uint32           _offset1;
} RBindDescriptorSetsInfo;

#if 0
typedef struct RBindResourceViewInfo {
	RResourceView* rv;
	// Temporary solution
	union {
		Uint16 slot;
		struct {
			Uint8 set;
			Uint8 binding;
		};
	};
	Uint16         target; // Pipeline type
	Uint32 	       type;
} RBindResourceViewInfo;
#endif

typedef struct RUpdateBufferInfo {
	RBuffer* handle;
	Uint32 offset;
	Uint32 length;
	void* data;
} RUpdateBufferInfo;

#define RG_SAMPLER_ADDRESSMODE_REPEAT        0x0
#define RG_SAMPLER_ADDRESSMODE_MIRRORED      0x1
#define RG_SAMPLER_ADDRESSMODE_CLAMP_TO_EDGE 0x2

#define RG_SAMPLER_FILTER_NEAREST            0x0
#define RG_SAMPLER_FILTER_LINEAR             0x1
#define RG_SAMPLER_FILTER_ANISOTROPIC        0x2

typedef struct RSamplerCreateInfo {
	Uint8 addressModeU;
	Uint8 addressModeV;
	Uint8 addressModeW;
	Uint8 filterMode;
	Uint8 maxAnisotropy;
	Uint8 _padding[3];
} RSamplerCreateInfo;

////////////////////////////////////////////////////////////////////

typedef struct R2D_Texture R2D_Texture;
typedef struct R2D_Buffer R2D_Buffer;

typedef struct R3D_Material R3D_Material;
typedef struct R3D_StaticModel R3D_StaticModel;
typedef struct R3D_RiggedModel R3D_RiggedModel;
typedef struct R3D_BoneBuffer R3D_BoneBuffer;
typedef struct R3D_AtlasHandle R3D_AtlasHandle;
typedef struct R3D_ParticleBuffer R3D_ParticleBuffer;

typedef struct RenderSetupInfo {

	Uint32 flags;
	
	// TODO

} RenderSetupInfo;

typedef struct RenderInfo {
	String render_name; // Name in renderer module
	String renderer;    // Graphics card name

	// Memory
	Uint64 shared_memory;    // 0 - Unknown
	Uint64 dedicated_memory; // 0 - Unknown
	Uint64 textures_memory;  // Textures memory
	Uint64 buffers_memory;   // Buffers memory

	// Textures
	Uint32 textures_left;
	Uint32 textures_inQueue;
	Uint32 textures_loaded;

	// Meshes
	Uint32 meshes_loaded;

	// Action
	Uint32 r3d_draw_calls;
	Uint32 r3d_dispatch_calls;

	////////////////
	void* r3d_renderResult; // (OpenGL - GLuint, D3D11 - ID3D11ShaderResourceView*, Vulkan - VkDescriptorSet)
	void* r2d_renderResult;

} RenderInfo;

/////////////////////
// R2D
/////////////////////

typedef struct R2D_Vertex {
	vec2 pos;
	vec2 uv;
	vec4 color;
} R2D_Vertex;

typedef struct R2DCreateBufferInfo {
	Uint32      length; // buffer length IN VERTICES (NOT BYTES)
	R2D_Vertex* initial_data;
} R2DCreateBufferInfo;

typedef struct R2DBufferDataInfo {
	R2D_Buffer* buffer;
	Uint32      offset; // offset IN VERTICES (NOT BYTES)
	Uint32      length; // data length IN VERTICES (NOT BYTES)
	R2D_Vertex* data;
} R2DBufferDataInfo;

typedef struct R2DCreateTextureInfo {
	String path;
} R2DCreateTextureInfo;

typedef struct R2DCreateMemTextureInfo {
	TextureType type;
	Uint32      width;
	Uint32      height;
	void*       data;
} R2DCreateMemTextureInfo;

typedef struct R2DTextureDataInfo {
	// Not implemented yet
	R2D_Texture* handle;
	void* data;
} R2DTextureDataInfo;

typedef struct R2DBindInfo {
	R2D_Texture* texture;
	R2D_Buffer*  buffer;
	vec4         color;
} R2DBindInfo;

typedef struct R2DDrawInfo {
	Uint32 offset; // offset IN "R2D_Vertex"
	Uint32 count;
} R2DDrawInfo;

/////////////////////
// R3D
/////////////////////

// R3D_GlobalLightDescription
typedef struct R3D_GlobalLightDescrition {
	vec3    color;
	Float32 time;
	Float32 intensity;
	Float32 ambient;
	Float32 turbidity; // skybox
} R3D_GlobalLightDescrition;

typedef struct R3D_RenderTaskInfo {
	R3D_GlobalLightDescrition* globallight;
} R3D_RenderTaskInfo;

typedef struct R3D_Vertex {
	vec3 pos;
	vec3 norm;
	vec3 tang;
	vec2 uv;
} R3D_Vertex;

typedef struct R3D_Weight {
	vec4  weight;
	ivec4 idx;
} R3D_Weight;

typedef struct Particle {
	vec3    pos;
	Float32 lifetime;
	vec3    vel;
	Float32 mul; // (>1 - Increase velocity, <1 - decrease velocity)
} Particle;

typedef struct R3D_MeshInfo {
	R3D_Material* material;
	Uint32        indexOffset;
	Uint32        indexCount;
} R3D_MeshInfo;

typedef struct R3D_LightSource {
	LightType type;
	vec3      color;
	vec3      position;
	Float32   intensity;

	// Spotlight
	vec3      direction;
	Float32   innerCone;
	Float32   outerCone;
} R3D_LightSource;

// Info structures
typedef struct R3DCreateMaterialInfo {
	String texture;
	vec3   color;
#if 0
	String albedo; //
	String normal; //
	String pbr;    //
#endif
} R3DCreateMaterialInfo;

typedef struct R3D_MaterialInfo {
	char   texture[128];
	vec3   color;
#if 0
	char   albedo[128];
	char   normal[128];
	char   pbr[128];
	vec3   color;
#endif
} R3D_MaterialInfo;

typedef struct R3D_MatMeshInfo {
	Uint32 materialIdx;
	Uint32 indexOffset;
	Uint32 indexCount;
} R3D_MatMeshInfo;

typedef struct R3DStaticModelInfo {
	R3D_MaterialInfo* matInfo;
	Uint32            matCount;

	R3D_MatMeshInfo* mInfo;
	Uint32           mCount;

	R3D_Vertex*   vertices;
	void*         indices;
	Uint32        vCount;
	Uint32        iCount;
	IndexType     iType;

	AABB          aabb;
} R3DStaticModelInfo;

typedef struct R3DRiggedModelInfo {
	R3D_MaterialInfo* matInfo;
	Uint32            matCount;

	R3D_MatMeshInfo* mInfo;
	Uint32           mCount;

	R3D_Vertex*   vertices;
	R3D_Weight*   weights;
	void*         indices;
	Uint32        vCount;
	Uint32        iCount;
	IndexType     iType;

	AABB          aabb;
} R3DRiggedModelInfo;

typedef struct R3DCreateBufferInfo {
	Uint32 len;
	void*  initialData;
} R3DCreateBufferInfo;

typedef struct R3DUpdateBufferInfo {
	union {
		void* handle;
		R3D_BoneBuffer*     handle_bone;
		R3D_ParticleBuffer* handle_particle;
	};
	Uint32 offset;
	Uint32 length;
	void*  data;
} R3DUpdateBufferInfo;

typedef struct R3D_PushModelInfo {
	union {
		void*            handle;
		R3D_StaticModel* handle_static;
		R3D_RiggedModel* handle_rigged;
	};
	R3D_BoneBuffer*      handle_bonebuffer;
	mat4 matrix;
} R3D_PushModelInfo;

typedef struct R3D_CameraInfo {
	mat4 projection;
	//mat4 view;
	vec3 position;
	Float32 _offset0;
	vec3 rotation;
	Float32 _offset1;
} R3D_CameraInfo;

#endif