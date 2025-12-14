#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "rendertypes.h"

typedef struct Texture {
	RImage* img;
	RResourceView* srv;
	Uint32 refcounter;
} Texture;

namespace Engine {
	namespace Render {
		
		void InitializeTextures();
		void DestroyTextures();

		Texture* GetTexture(String path);
		void FreeTexture(Texture* tex);

		void DoLoadTextures();

		Texture* GetDefaultWhiteTexture();
		Texture* GetDefaultNormalTexture();
		Texture* GetDefaultPBRTexture();

	}
}

#endif
