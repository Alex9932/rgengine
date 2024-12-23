#if 1


#include "loader.h"
#include "rgrenderdx11.h"
#include "texture.h"
#include "queue.h"

#include <allocator.h>
#include <rgstb.h>

#include <rgmath.h>

#include <map>

#define R_TX_IN_QUEUE 65536

static Texture* DEFAULT_TEXTURE;

static std::map<Uint32, Texture*> loaded_textures;

static Engine::PoolAllocator* t_pool = NULL;
static RQueue* t_queue = NULL;

struct TexturePoolBlock {
	char      path[120];
	Texture** pointer;
};

void LoaderInit() {
	TextureInfo tInfo = {};
	Uint32 tdata = 0xFFFFFFFF;
	tInfo.width = 1;
	tInfo.height = 1;
	tInfo.channels = 4;
	tInfo.data = &tdata;
	DEFAULT_TEXTURE = RG_NEW_CLASS(RGetAllocator(), Texture)(&tInfo);

	t_pool = new Engine::PoolAllocator("Texture queue", R_TX_IN_QUEUE, sizeof(TexturePoolBlock));
	t_queue = RG_NEW_CLASS(RGetAllocator(), RQueue)(R_TX_IN_QUEUE);

}

void LoaderDestroy() {
	delete t_pool;
	RG_DELETE_CLASS(RGetAllocator(), RQueue, t_queue);

	RG_DELETE_CLASS(RGetAllocator(), Texture, DEFAULT_TEXTURE);
}

Texture* GetDefaultTexture() {
	return DEFAULT_TEXTURE;
}

void LoaderPushTexture(String path, Texture** tptr) {

	Uint32 hash = rgCRC32(path, SDL_strlen(path));

	if (loaded_textures[hash] != NULL) {
		Texture* tx = loaded_textures[hash];
		tx->references++;
		*tptr = tx;
		return;
	}
	
	*tptr = DEFAULT_TEXTURE;

	TexturePoolBlock* block = (TexturePoolBlock*)t_pool->Allocate();
	block->pointer = tptr;
	SDL_snprintf(block->path, 120, "%s", path);
	t_queue->Push(block);

}

void DoLoadTextures() {
	if (t_queue->Size() == 0) {
		return;
	}

	TexturePoolBlock* block = (TexturePoolBlock*)t_queue->Next();

	// End of queue
	if (!block) {
		t_queue->Clear();
		return;
	}

	Uint32 hash = rgCRC32(block->path, SDL_strlen(block->path));

	// If texture already loaded
	if (loaded_textures[hash] != NULL) {
		Texture* tx = loaded_textures[hash];
		tx->references++;
		*block->pointer = tx;

		t_pool->Deallocate(block);
		return;
	}

	int w, h, c;
	Uint8* data = RG_STB_load_from_file(block->path, &w, &h, &c, 4);
	TextureInfo albedoInfo = {};
	albedoInfo.width    = w;
	albedoInfo.height   = h;
	albedoInfo.channels = 4;
	albedoInfo.data     = data;

	Texture* tx = RG_NEW_CLASS(RGetAllocator(), Texture)(&albedoInfo);
	tx->references++;

	loaded_textures[hash] = tx;

	*block->pointer = tx;


	RG_STB_image_free((Uint8*)albedoInfo.data);

	t_pool->Deallocate(block);
}

void TexturesDelete(Texture* tx) {

	if (tx == DEFAULT_TEXTURE) { return; }

	tx->references--;
	if (tx->references == 0) {
		RG_DELETE_CLASS(RGetAllocator(), Texture, tx);
	}
}

Uint32 TexturesInQueue() {
	return t_queue->Size() - t_queue->Current();
}

Uint32 TexturesLeft() {
	return t_queue->Size();
}


#else

#include "loader.h"
#include "rgrenderdx11.h"
#include "texture.h"
#include "queue.h"

#include <allocator.h>
#include <rgstb.h>

static Texture* DEFAULT_TEXTURE;

static Engine::PoolAllocator* t_pool  = NULL;
static RQueue*                t_queue = NULL;

struct TexturePoolBlock {
	char      path[120];
	Texture** pointer;
};

void LoaderInit() {
	TextureInfo tInfo = {};
	Uint32 tdata = 0xFFFFFFFF;
	tInfo.width    = 1;
	tInfo.height   = 1;
	tInfo.channels = 4;
	tInfo.data     = &tdata;
	DEFAULT_TEXTURE = RG_NEW_CLASS(RGetAllocator(), Texture)(&tInfo);

	t_pool  = new Engine::PoolAllocator("Texture queue", 4096, sizeof(TexturePoolBlock));
	t_queue = RG_NEW_CLASS(RGetAllocator(), RQueue)(4096);

}

void LoaderDestroy() {
	delete t_pool;
	RG_DELETE_CLASS(RGetAllocator(), RQueue, t_queue);

	RG_DELETE_CLASS(RGetAllocator(), Texture, DEFAULT_TEXTURE);
}

Texture* GetDefaultTexture() {
	return DEFAULT_TEXTURE;
}

void LoaderPushTexture(String path, Texture** tptr) {
	*tptr = DEFAULT_TEXTURE;

	TexturePoolBlock* block = (TexturePoolBlock*)t_pool->Allocate();
	block->pointer = tptr;
	SDL_snprintf(block->path, 120, "%s", path);
	t_queue->Push(block);

}

void DoLoadTextures() {
	if (t_queue->Size() == 0) {
		return;
	}

	TexturePoolBlock* block = (TexturePoolBlock*)t_queue->Next();

	// End of queue
	if (!block) {
		t_queue->Clear();
		return;
	}

	int w, h, c;
	Uint8* data = RG_STB_load_from_file(block->path, &w, &h, &c, 4);
	TextureInfo albedoInfo = {};
	albedoInfo.width    = w;
	albedoInfo.height   = h;
	albedoInfo.channels = c;
	albedoInfo.data     = data;

	*block->pointer = RG_NEW_CLASS(RGetAllocator(), Texture)(&albedoInfo);

	RG_STB_image_free((Uint8*)albedoInfo.data);

	t_pool->Deallocate(block);
}

Uint32 TexturesInQueue() {
	return t_queue->Size() - t_queue->Current();
}

Uint32 TexturesLeft() {
	return t_queue->Size();
}

#endif