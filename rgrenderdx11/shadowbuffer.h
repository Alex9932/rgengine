#ifndef _SHADOWBUFFER_H
#define _SHADOWBUFFER_H

#include <rgvector.h>
#include <d3d11.h>

//ID3D11ShaderResourceView* GetShadowBufferShaderResource(Uint32 idx);
ID3D11ShaderResourceView* GetShadowDepth();

void CreateShadowBuffer(ivec2* size);
void DestroyShadowBuffer();
void ResizeShadowBuffer(ivec2* size);
void BindShadowBuffer();

#endif