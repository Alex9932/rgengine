#ifndef _TEXTURE_H
#define _TEXTURE_H

#include <d3d11.h>
#include <rgtypes.h>

struct TextureInfo {
	Uint32 width;
	Uint32 height;
	Uint32 channels;
	Uint8* data;
};

class Texture {
	public:
		Texture(TextureInfo* info);
		virtual ~Texture();

		void Bind(Uint32 bindPoint);

	private:
		ID3D11ShaderResourceView* shaderResource = NULL;
		ID3D11Texture2D* texture = NULL;
};


#endif