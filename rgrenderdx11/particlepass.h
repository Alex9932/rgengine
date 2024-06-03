#ifndef _PARTICLEPASS_H
#define _PARTICLEPASS_H

#include "rgvector.h"
#include "dx11.h"

ID3D11ShaderResourceView* GetParticleShaderResource();

void CreateParticlePass(ivec2* size);
void DestroyParticlePass();
void ResizeParticlePass(ivec2* size);
void ReloadParticleShaders();

void RenderParticles();

#endif