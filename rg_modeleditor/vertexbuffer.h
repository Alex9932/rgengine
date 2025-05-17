#ifndef _VERTEXBUFFER_H
#define _VERTEXBUFFER_H

#include <rendertypes.h>
#include "texture.h"

typedef struct VertexBuffer;

struct RenderState;

VertexBuffer* GetVertexbuffer();
void MakeVBuffer(R3DStaticModelInfo* info, String mdlpath);
void FreeVBuffer(VertexBuffer* ptr);
void DrawBuffer(RenderState* state, VertexBuffer* ptr);

#endif