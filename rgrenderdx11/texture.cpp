#include "texture.h"
#include "dx11.h"
#include <engine.h>

static Uint64 texturesMemory = 0;

static inline DXGI_FORMAT GetTextureFormat(Uint32 channels) {
	switch (channels) {
		case 1:  return DXGI_FORMAT_R8_UNORM;
		case 2:  return DXGI_FORMAT_R8G8_UNORM;
		//case 3:  return DXGI_FORMAT_B8G8R8X8_UNORM;
		case 4:  return DXGI_FORMAT_R8G8B8A8_UNORM;
		default: return DXGI_FORMAT_R8G8B8A8_UNORM;
	}
}

Texture::Texture(TextureInfo* info) {

	if (info->channels == 3) {
		RG_ERROR_MSG("D3D11 Not support 3-channel textures!");
	}

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = info->width;
	textureDesc.Height = info->height;
	textureDesc.MipLevels = 0;
	textureDesc.ArraySize = 1;
	textureDesc.Format = GetTextureFormat(info->channels);
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	DX11_GetDevice()->CreateTexture2D(&textureDesc, NULL, &this->texture);

	RG_ASSERT_MSG(this->texture, "Unable to load texture: CreateTexture2D");

	int rowPitch = (info->width * info->channels);
	DX11_GetContext()->UpdateSubresource(this->texture, 0, NULL, info->data, rowPitch, 0);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	DX11_GetDevice()->CreateShaderResourceView(this->texture, &srvDesc, &this->shaderResource);
	DX11_GetContext()->GenerateMips(this->shaderResource);

	this->memLength = info->width * info->height * info->channels;

	texturesMemory += this->memLength;
}

Texture::~Texture() {
	this->texture->Release();
	this->shaderResource->Release();

	texturesMemory -= this->memLength;
}

void Texture::Bind(Uint32 idx) {
	DX11_GetContext()->PSSetShaderResources(idx, 1, &this->shaderResource);
}

Uint64 GetTextureMemory() {
	return texturesMemory;
}