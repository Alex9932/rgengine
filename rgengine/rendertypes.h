#ifndef _RENDERTYPES_H
#define _RENDERTYPES_H

#include "rgtypes.h"
#include "rgvector.h"
#include "rgmatrix.h"

enum IndexType {
	RG_INDEX_U32 = 4,
	RG_INDEX_U16 = 2,
	RG_INDEX_U8  = 1  // !!! DO NOT RECOMENDED TO USE !!!
};

// Backend handle
typedef struct R3D_Material R3D_Material;
typedef struct R3D_StaticModel R3D_StaticModel;
typedef struct R3D_RiggedModel R3D_RiggedModel;
typedef struct R3D_BoneBuffer R3D_BoneBuffer;

//

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

typedef struct R3DCreateStaticModelInfo {
	R3D_MeshInfo* info;
	R3D_Vertex*   vertices;
	void*         indices;
	Uint32        vCount;
	Uint32        iCount;
	Uint32        mCount;
	IndexType     iType;
} R3DCreateStaticModelInfo;

typedef struct R3DCreateRiggedModelInfo {
	R3D_MeshInfo* info;
	R3D_Vertex*   vertices;
	R3D_Weight*   weights;
	void*         indices;
	Uint32        vCount;
	Uint32        iCount;
	Uint32        mCount;
	IndexType     iType;
} R3DCreateRiggedModelInfo;

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