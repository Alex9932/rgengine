#ifndef _R2D_H
#define _R2D_H

#include <rendertypes.h>
#include <rgvector.h>

#include "buffer.h"
#include "texture.h"

struct R2D_Texture {
	Texture* texture;
};

struct R2D_Buffer {
	Buffer* buffer;
	Uint32  vertices;
};

void InitializeR2D(ivec2* size);
void DestroyR2D();
void ResizeR2D(ivec2* wndSize);

void ReloadShadersR2D();

ID3D11ShaderResourceView* R2DGetOuputTexture();

#endif