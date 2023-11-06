#ifndef _LIGHTPASS_H
#define _LIGHTPASS_H

#include <rgvector.h>
//#include "dx11.h"

void CreateLightpass(ivec2* size);
void DestroyLightpass();
void ResizeLightpass(ivec2* size);

void DoLightpass();

struct ID3D11ShaderResourceView* GetLightpassShaderResource();

#endif