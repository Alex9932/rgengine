#ifndef _VERTEXBUFFER_H
#define _VERTEXBUFFER_H

#include <rendertypes.h>
#include "texture.h"

typedef struct VertexBuffer;

VertexBuffer* MakeVBuffer(R3DStaticModelInfo* info);
void FreeVBuffer(VertexBuffer* ptr);
void DrawBuffer(VertexBuffer* ptr);


#endif