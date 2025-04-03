#ifndef _RENDERER_H
#define _RENDERER_H

// Custom renderer
#include <camera.h>

struct RenderState;

typedef void (*GuiDrawCallback)(void);

RenderState* InitializeRenderer(GuiDrawCallback guicb);
void DestroyRenderer(RenderState* state);
void DoRender(RenderState* state, Engine::Camera* cam);

#endif