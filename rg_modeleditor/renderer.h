#ifndef _RENDERER_H
#define _RENDERER_H

// Custom renderer
#include <camera.h>

struct RenderState;
struct VertexBuffer;

typedef void (*GuiDrawCallback)(void);

RenderState* InitializeRenderer(GuiDrawCallback guicb);
void DestroyRenderer(RenderState* state);
void DoRender(RenderState* state, Engine::Camera* cam);
void GetRenderSize(RenderState* state, ivec2* dst);
void ResizeRender(RenderState* state);
Bool* GetRenderWireframe(RenderState* state);

VertexBuffer* GetVertexbuffer();

#endif