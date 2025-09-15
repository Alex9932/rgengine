#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "glad.h"
#include <rgtypes.h>

typedef struct Texture {
	GLuint tex_id;
	Uint32 refcount;
	char tex_name[248];
} Texture;

void InitializeTextures();
void DestroyTextures();

Texture* GetTexture(String path);
void FreeTexture(Texture* tx);
void FreeAllTextures();

#endif