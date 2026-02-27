#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "rendertypes.h"

typedef void (*PFN_TEXTURELOADED)(void* userdata);

typedef struct Texture {
	RImage* img;
	Uint32  isLoaded;
	Uint32  refcounter;
} Texture;

namespace Engine {
	namespace Render {
		
		void InitializeTextures();
		void DestroyTextures();

		Texture* GetTexture(String path, PFN_TEXTURELOADED callback = NULL, void* userdata = NULL);
		void FreeTexture(Texture* tex);

		void DoLoadTextures();

		Texture* GetDefaultWhiteTexture();
		Texture* GetDefaultNormalTexture();
		Texture* GetDefaultPBRTexture();

	}
}

#endif
