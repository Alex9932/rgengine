#include "texture.h"
#include <allocator.h>
#include <rgmath.h>
#include <rgstb.h>
#include <map>

#define TEXMAP std::map<Uint64, Texture*>

static Engine::PoolAllocator* talloc;
static TEXMAP textures;

void InitializeTextures() {
	talloc = new Engine::PoolAllocator("Texture pool", 256, sizeof(Texture));
	textures.clear();
}

void DestroyTextures() {
	delete talloc;
}

Texture* GetTexture(String path) {
	Uint32 len = SDL_strlen(path);
	Uint64 hash = rgHash(path, len);

	Texture* texture = NULL;

	if (textures.count(hash) != 0) {
		texture = textures[hash];
		texture->refcount++;
		return texture;
	}
	
	// Load texture
	texture = (Texture*)talloc->Allocate();
	texture->refcount = 1;

	SDL_snprintf(texture->tex_name, 248, "%s", path);

	glGenTextures(1, &texture->tex_id);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	Uint8* data = RG_STB_load_from_file(path, &width, &height, &nrChannels, 4);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		rgLogError(RG_LOG_RENDER, "Texture loading error!");
	}
	RG_STB_image_free(data);

	textures[hash] = texture;
	return texture;
}

void FreeTexture(Texture* tx) {
	tx->refcount--;
	if (tx->refcount == 0) {
		rgLogInfo(RG_LOG_RENDER, "Delete texture %s", tx->tex_name);

		//Delete from map
		TEXMAP::iterator it = textures.begin();
		for (; it != textures.end(); it++) {
			if (it->second == tx) {
				textures.erase(it);
				break;
			}
		}

		glDeleteTextures(1, &tx->tex_id);
		talloc->Deallocate(tx);
	}
}