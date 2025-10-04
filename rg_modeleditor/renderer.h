#ifndef _RENDERER_H
#define _RENDERER_H

// Custom renderer
#include <camera.h>

struct RenderState;
struct VertexBuffer;

namespace Engine {
	class KinematicsModel;
};

typedef void (*GuiDrawCallback)(void);

RenderState* InitializeRenderer(GuiDrawCallback guicb);
void DestroyRenderer(RenderState* state);
void DoRender(RenderState* state, Engine::Camera* cam);
void GetRenderSize(RenderState* state, ivec2* dst);
void ResizeRender(RenderState* state);
Bool* GetRenderWireframe(RenderState* state);
Bool* GetRenderSkeleton(RenderState* state);
Bool* GetRenderShowmesh(RenderState* state);

void SetRenderKModel(RenderState* state, Engine::KinematicsModel* mdl);

void CalculateModelMatrix(RenderState* state, mat4* m);
vec3* GetRenderMdlposPtr(RenderState* state);
vec3* GetRenderMdlrotPtr(RenderState* state);
vec3* GetRenderMdlsizePtr(RenderState* state);

mat4* GetRenderBoneMatPtr(RenderState* state);

void SetMaterialState(RenderState* state, VertexBuffer* vb, Uint32 mat);

VertexBuffer* GetVertexbuffer();

#endif