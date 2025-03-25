#ifndef _RENDERER_H
#define _RENDERER_H

// Custom renderer

struct RenderState;

RenderState* InitializeRenderer();
void DestroyRenderer(RenderState* state);
void DoRender(RenderState* state);

#endif