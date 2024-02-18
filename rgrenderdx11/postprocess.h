#ifndef _POSTPROCESS_H
#define _POSTPROCESS_H

#include "dx11.h"

void CreateFX(ivec2* size);
void DestroyFX();
void ResizeFX(ivec2* size);
void ReloadShadersFX();

void DoPostprocess();

void SetViewIDX(Uint32 idx);
ID3D11ShaderResourceView* FXGetOuputTexture();

#endif