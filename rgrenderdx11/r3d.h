#ifndef _R3D_H
#define _R3D_H

#include <rendertypes.h>
#include <allocator.h>
#include "buffer.h"
#include "texture.h"
#include "rgvector.h"
#include "queue.h"

enum ModelType {
	R_MODEL_STATIC = 0,
	R_MODEL_RIGGED = 1
};

typedef struct R3D_Material {
	Texture* albedo;
	Texture* normal;
	Texture* pbr;
	vec3     color;
} R3D_Material;

typedef struct R3D_StaticModel {
	ModelType     type;
	Uint32        mCount;
	R3D_MeshInfo* info;
	Buffer*		  vBuffer;
	Buffer*		  iBuffer;
	Uint32        iCount;
	IndexType     iType;
} R3D_StaticModel;

typedef struct R3D_RiggedModel {
	ModelType                  type;
	// Input data
	Buffer*                    i_vertex;  // Output vertex data
	Buffer*                    i_weight;  // Output vertex data
	ID3D11ShaderResourceView*  i_srv_vtx; // Shader resource view for vertex input data
	ID3D11ShaderResourceView*  i_srv_wht; // Shader resource view for weigth input data

	// Output data
	R3D_StaticModel            s_model;   // Static model / output vertex data
	ID3D11UnorderedAccessView* s_srv;     // Unordered access view for vertex output data
} R3D_RiggedModel;

typedef struct R3D_BoneBuffer {
	Buffer*                   bBuffer;
	ID3D11ShaderResourceView* resourceView;
} R3D_BoneBuffer;

void InitializeR3D(ivec2* size);
void DestroyR3D();
void ResizeR3D(ivec2* wndSize);

RQueue* GetStaticQueue();
RQueue* GetRiggedQueue();

mat4* GetCameraProjection();
mat4* GetCameraView();
vec3* GetCameraPosition();
vec3* GetCameraRotation();

Engine::LinearAllocator* GetMatrixAllocator();

#endif