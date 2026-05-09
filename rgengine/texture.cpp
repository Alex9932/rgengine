#include "texture.h"
#include "allocator.h"
#include "render.h"

#include "queue.h"
#include "rgstb.h"

#include "rgthread.h"

#include <map>

#define R_MAX_TEXTURES 64000
#define R_MAX_LOADQUEUE 4000

typedef struct TextureInfo {
	Texture* tex;
	void*    userdata;
	PFN_TEXTURELOADED callback;
	char   path[248];
} TextureInfo;

namespace Engine {
	namespace Render {

		static PoolAllocator* texturespool = NULL;
		static PoolAllocator* texinfopool  = NULL;
		static std::map<Uint64, Texture*> texturescache;
		static Queue* textureloadqueue = NULL;

		static Texture* tx_white  = NULL;
		static Texture* tx_normal = NULL;
		static Texture* tx_pbr    = NULL;

		static void ImmediateLoadTexture(String path, Texture* tptr) {
			RRenderDevice* rdev = GetRenderDevice();
			RenderBackend* rctx = GetRenderContext();

			// Load texture from file
			int w, h, c;
			Uint8* data = RG_STB_load_from_file(path, &w, &h, &c, 4);

			RImageCreateInfo imginfo = {};
			imginfo.width = w;
			imginfo.height = h;
			imginfo.format = RG_FORMAT_R8G8B8A8_UNORM;
			imginfo.flags = tptr->flags;
			imginfo.initialData = data;

			tptr->img = rctx->CreateImage(rdev, &imginfo);

			RG_STB_image_free((Uint8*)data);
			tptr->isLoaded = true;

		}

		static void DefferedLoadTexture(String path, Texture* tptr) {
			Task task = {};
			//task.proc = LoadTask;
//			task.userdata = info;

			ThreadDispatch(&task, RG_TASK_ASYNC);
		}

		static Texture* CreateTexture(String path, PFN_TEXTURELOADED loadcallback, void* userdata, Uint16 flags) {
			Texture* tex = (Texture*)texturespool->Allocate();
			tex->refcounter = 1;
			tex->img = NULL;
			tex->flags = flags;
			tex->isLoaded = false;

			TextureInfo* info = (TextureInfo*)texinfopool->Allocate();
			SDL_snprintf(info->path, 248, "%s", path);
			info->tex = tex;
			info->callback = loadcallback;
			info->userdata = userdata;
			textureloadqueue->Push(info);
			return tex;
		}

		static void DestroyTexture(Texture* tex) {
			RenderBackend* rctx = GetRenderContext();
			//rctx->DestroyResourceView(tex->srv);
			rctx->DestroyImage(tex->img);
			texturespool->Deallocate(tex);
		}

		void InitializeTextures() {
			texturescache.clear();
			texturespool     = RG_NEW(PoolAllocator)("Texture Pool Allocator", R_MAX_TEXTURES, sizeof(Texture));
			texinfopool      = RG_NEW(PoolAllocator)("Texture Load Pool", R_MAX_LOADQUEUE, sizeof(TextureInfo));
			textureloadqueue = RG_NEW(Queue)(R_MAX_LOADQUEUE);

			tx_white = (Texture*)texturespool->Allocate();
			tx_white->refcounter = 1;
			tx_normal = (Texture*)texturespool->Allocate();
			tx_normal->refcounter = 1;
			tx_pbr = (Texture*)texturespool->Allocate();
			tx_pbr->refcounter = 1;

			ImmediateLoadTexture("platform/textures/def_diffuse.png", tx_white);
			ImmediateLoadTexture("platform/textures/def_normal.png", tx_normal);
			ImmediateLoadTexture("platform/textures/def_pbr.png", tx_pbr);
		}

		void DestroyTextures() {
			DestroyTexture(tx_white);
			DestroyTexture(tx_normal);
			DestroyTexture(tx_pbr);
			RG_DELETE(PoolAllocator, texturespool);
			RG_DELETE(PoolAllocator, texinfopool);
			RG_DELETE(Queue, textureloadqueue);
		}

		Texture* GetTexture(String path, PFN_TEXTURELOADED loadcallback, void* userdata, Uint16 flags) {
			Uint64 hash = rgHash(path, SDL_strlen(path));
			Texture* tex = NULL;
			// Return loaded material
			if (texturescache.count(hash) != 0) {
				tex = texturescache[hash];
				tex->refcounter++;
				return tex;
			}

			// Load new texture
			tex = CreateTexture(path, loadcallback, userdata, flags);
			texturescache[hash] = tex;
			return tex;
		}

		void FreeTexture(Texture* tex) {
			tex->refcounter--;
			if (tex->refcounter == 0) {
				// Delete texture
				for (auto it = texturescache.begin(); it != texturescache.end();) {
					if (it->second == tex) {
						texturescache.erase(it++);
					} else {
						++it;
					}
				}
				DestroyTexture(tex);
			}
		}

		static void LoadTask(void* userdata) {
			TextureInfo* info = (TextureInfo*)userdata;
			// TODO: add deffered texture loading
			ImmediateLoadTexture(info->path, info->tex);
			if (info->callback) {
				info->callback(info->userdata);
			}
			texinfopool->Deallocate(info);
		}

		void DoLoadTextures() {
			TextureInfo* info = (TextureInfo*)textureloadqueue->Pop();
			if (!info) { return; } // No textures to load

			Task task = {};
			task.proc = LoadTask;
			task.userdata = info;
			if (!ThreadDispatch(&task, RG_TASK_ASYNC)) {
				textureloadqueue->Push(info);
			}
		}

		Texture* GetDefaultWhiteTexture()  { return tx_white; }
		Texture* GetDefaultNormalTexture() { return tx_normal; }
		Texture* GetDefaultPBRTexture()    { return tx_pbr; }

	}
}