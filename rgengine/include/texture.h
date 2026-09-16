#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "rendertypes.h"

typedef void (*PFN_TEXTURELOADED)(void* userdata);

typedef struct Texture {
	RImage* img;
	Uint8   isLoaded;
	Uint8   flags;
	Uint16  _off1;
	Uint32  refcounter;
} Texture;

namespace Engine {
	namespace Render {
		
		void InitializeTextures();
		void DestroyTextures();

		Texture* GetTexture(String path, PFN_TEXTURELOADED callback = NULL, void* userdata = NULL, Uint16 flags = 0);
		void FreeTexture(Texture* tex);

		void DoLoadTextures();

		Texture* GetDefaultWhiteTexture();
		Texture* GetDefaultNormalTexture();
		Texture* GetDefaultPBRTexture();

	}
}

#endif
