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