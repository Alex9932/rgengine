#ifndef _VERTEXBUFFER_H
#define _VERTEXBUFFER_H

#include <rendertypes.h>
#include "texture.h"

#define VERTEXBUFFER_MATERIALS 512

typedef struct IndexPair {
	Uint32 start;
	Uint32 count;
	Bool   flipuv; // flip UVs
} IndexPair;

typedef struct VertexBuffer {
	Bool      isLoaded;
	GLuint    vbo;
	GLuint    vbo_w;
	GLuint    ebo;
	GLuint    vao;
	Uint32    meshes;
	Uint32    tcount;
	Uint32    indexsize;
	Uint32    mat[VERTEXBUFFER_MATERIALS]; // material index
	IndexPair pairs[VERTEXBUFFER_MATERIALS];
	Texture*  textures[VERTEXBUFFER_MATERIALS * 3];
	vec3      colors[VERTEXBUFFER_MATERIALS];
} VertexBuffer;

struct RenderState;

VertexBuffer* GetVertexbuffer();
void MakeVBuffer(R3DRiggedModelInfo* info, String mdlpath);
void FreeVBuffer(VertexBuffer* ptr);
void DrawBuffer(RenderState* state, VertexBuffer* ptr);

#endif