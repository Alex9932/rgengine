#ifndef _GBUFFER_H
#define _GBUFFER_H

#include <rgvector.h>
#include <d3d11.h>

ID3D11ShaderResourceView* GetGBufferShaderResource(Uint32 idx);

void CreateGBuffer(ivec2* size);
void DestroyGBuffer();
void ResizeGbuffer(ivec2* size);
void BindGBuffer();



#endif