#ifndef _LOADER_H
#define _LOADER_H

#include <rgtypes.h>
#include "texture.h"

void LoaderInit();
void LoaderDestroy();

Texture* GetDefaultTexture();

void LoaderPushTexture(String path, Texture** tptr);

void TexturesDelete(Texture* tx);

void DoLoadTextures();
Uint32 TexturesInQueue();
Uint32 TexturesLeft();

#endif