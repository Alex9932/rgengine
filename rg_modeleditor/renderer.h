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

void CalculateModelMatrix(RenderState* state, mat4* m);
vec3* GetRenderMdlposPtr(RenderState* state);
vec3* GetRenderMdlrotPtr(RenderState* state);
vec3* GetRenderMdlsizePtr(RenderState* state);

void SetMaterialState(RenderState* state, VertexBuffer* vb, Uint32 mat);

VertexBuffer* GetVertexbuffer();

#endif