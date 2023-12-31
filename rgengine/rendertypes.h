#ifndef _RENDERTYPES_H
#define _RENDERTYPES_H

#include "rgtypes.h"
#include "rgvector.h"
#include "rgmatrix.h"

#include "imgui/imgui.h"

enum IndexType {
	RG_INDEX_U32 = 4,
	RG_INDEX_U16 = 2,
	RG_INDEX_U8  = 1  // !!! DO NOT RECOMENDED TO USE !!!
};

typedef Uint32 RenderFlags;

#define RG_RENDER_FULLSCREEN 1
#define RG_RENDER_USE3D      2
#define RG_RENDER_RESERVED   4

// Backend handle
typedef struct R3D_Material R3D_Material;
typedef struct R3D_StaticModel R3D_StaticModel;
typedef struct R3D_RiggedModel R3D_RiggedModel;
typedef struct R3D_BoneBuffer R3D_BoneBuffer;

//

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

} RenderInfo;

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

typedef struct R3D_MeshInfo {
	R3D_Material* material;
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
} R3DRiggedModelInfo;

typedef struct R3DCreateBoneBufferInfo {
	Uint32 len;
	void*  initialData;
} R3DCreateBoneBufferInfo;

typedef struct R3DBoneBufferUpdateInfo {
	R3D_BoneBuffer* handle;
	Uint32          offset;
	Uint32          length;
	void*           data;
} R3DBoneBufferUpdateInfo;

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