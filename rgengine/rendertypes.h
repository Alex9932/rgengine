#ifndef _RENDERTYPES_H
#define _RENDERTYPES_H

#include "rgtypes.h"
#include "rgvector.h"
#include "rgmatrix.h"
#include "rgmath.h"

#include "imgui/imgui.h"

enum IndexType {
	RG_INDEX_U32 = 4,
	RG_INDEX_U16 = 2,
	RG_INDEX_U8  = 1  // !!! DO NOT RECOMENDED TO USE !!!
};

/*
enum TextureType {
	RG_TEXTURE_U8_R_ONLY  = 1,
	RG_TEXTURE_U8_RGBA    = 2,
	RG_TEXTURE_F32_R_ONLY = 3,
	RG_TEXTURE_F32_RGBA   = 4
};
*/

typedef Uint32 RenderFlags;

#define RG_RENDER_FULLSCREEN 1
#define RG_RENDER_USE3D      2
#define RG_RENDER_RESERVED   4
#define RG_RENDER_NOLIGHT    8

// Backend handles

typedef struct R2D_Texture R2D_Texture;
typedef struct R2D_Buffer R2D_Buffer;

typedef struct R3D_Material R3D_Material;
typedef struct R3D_StaticModel R3D_StaticModel;
typedef struct R3D_RiggedModel R3D_RiggedModel;
typedef struct R3D_BoneBuffer R3D_BoneBuffer;
typedef struct R3D_AtlasHandle R3D_AtlasHandle;
typedef struct R3D_ParticleBuffer R3D_ParticleBuffer;

typedef struct RenderSetupInfo {

	RenderFlags flags;
	
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
	void* r3d_renderResult; // (OpenGL - GLuint, D3D11 - ID3D11ShaderResourceView*)
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
	/*
	TextureType type;
	Uint32      width;
	Uint32      height;
	void*       data;
	*/
} R2DCreateTextureInfo;

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

// Info structures
typedef struct R3DCreateMaterialInfo {
	String albedo; //
	String normal; //
	String pbr;    //
	vec3   color;
} R3DCreateMaterialInfo;

typedef struct R3D_MaterialInfo {
	char   albedo[128];
	char   normal[128];
	char   pbr[128];
	vec3   color;
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
	vec3 position;
	vec3 rotation;
} R3D_CameraInfo;

#endif