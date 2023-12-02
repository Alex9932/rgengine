#ifndef _POSTPROCESS_H
#define _POSTPROCESS_H

#include "dx11.h"

void CreateFX(ivec2* size);
void DestroyFX();
void ResizeFX(ivec2* size);

void DoPostprocess();

ID3D11ShaderResourceView* FXGetOuputTexture();

#endif