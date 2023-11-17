#ifndef _LOADER_H
#define _LOADER_H

#include <rgtypes.h>
#include "texture.h"

void LoaderInit();
void LoaderDestroy();

Texture* GetDefaultTexture();

void LoaderPushTexture(String path, Texture** tptr);

void DoLoadTextures();

#endif