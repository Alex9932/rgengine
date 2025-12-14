#include "texture.h"
#include "allocator.h"
#include "render.h"

#include "queue.h"
#include "rgstb.h"

#include <map>

#define R_MAX_TEXTURES 64000
#define R_MAX_LOADQUEUE 4000

typedef struct TextureInfo {
	Texture* tex;
	char   path[248];
} TextureInfo;

namespace Engine {
	namespace Render {

		static PoolAllocator* texturespool = NULL;
		static PoolAllocator* texinfopool  = NULL;
		static std::map<Uint64, Texture*> texturescache;
		static Queue* textureloadqueue = NULL;

		static Texture* tx_while  = NULL;
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
			imginfo.initialData = data;

			tptr->img = rctx->CreateImage(rdev, &imginfo);

			RResourceViewCreateInfo srvinfo = {};
			srvinfo.type = RG_RESOURCEVIEW_TYPE_SRV;
			srvinfo.buffer_type = RG_RESOURCEVIEW_IMAGE;
			srvinfo.dst_image = tptr->img;
			tptr->srv = rctx->CreateResourceView(rdev, &srvinfo);

			RG_STB_image_free((Uint8*)data);
		}

		static Texture* CreateTexture(String path) {
			Texture* tex = (Texture*)texturespool->Allocate();
			tex->refcounter = 1;
			tex->img = NULL;
			tex->srv = NULL;

			TextureInfo* info = (TextureInfo*)texinfopool->Allocate();
			SDL_snprintf(info->path, 248, "%s", path);
			info->tex = tex;
			textureloadqueue->Push(info);
			return tex;
		}

		static void DestroyTexture(Texture* tex) {
			RenderBackend* rctx = GetRenderContext();
			rctx->DestroyResourceView(tex->srv);
			rctx->DestroyImage(tex->img);
			texturespool->Deallocate(tex);
		}

		void InitializeTextures() {
			texturescache.clear();
			texturespool     = RG_NEW(PoolAllocator)("Texture Pool Allocator", R_MAX_TEXTURES, sizeof(Texture));
			texinfopool      = RG_NEW(PoolAllocator)("Texture Load Pool", R_MAX_LOADQUEUE, sizeof(TextureInfo));
			textureloadqueue = RG_NEW(Queue)(R_MAX_LOADQUEUE);

			tx_while = (Texture*)texturespool->Allocate();
			tx_while->refcounter = 1;
			tx_normal = (Texture*)texturespool->Allocate();
			tx_normal->refcounter = 1;
			tx_pbr = (Texture*)texturespool->Allocate();
			tx_pbr->refcounter = 1;

			ImmediateLoadTexture("platform/textures/def_diffuse.png", tx_while);
			ImmediateLoadTexture("platform/textures/def_normal.png", tx_normal);
			ImmediateLoadTexture("platform/textures/def_pbr.png", tx_pbr);
		}

		void DestroyTextures() {
			DestroyTexture(tx_while);
			DestroyTexture(tx_normal);
			DestroyTexture(tx_pbr);
			RG_DELETE(PoolAllocator, texturespool);
			RG_DELETE(PoolAllocator, texinfopool);
			RG_DELETE(Queue, textureloadqueue);
		}

		Texture* GetTexture(String path) {
			Uint64 hash = rgHash(path, SDL_strlen(path));
			Texture* tex = NULL;
			// Return loaded material
			if (texturescache.count(hash) != 0) {
				tex = texturescache[hash];
				tex->refcounter++;
				return tex;
			}

			// Load new texture
			tex = CreateTexture(path);
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

		void DoLoadTextures() {
			TextureInfo* info = (TextureInfo*)textureloadqueue->Pop();
			if (!info) { return; } // No textures to load
			ImmediateLoadTexture(info->path, info->tex);
			texinfopool->Deallocate(info);
		}

		Texture* GetDefaultWhiteTexture()  { return tx_while; }
		Texture* GetDefaultNormalTexture() { return tx_normal; }
		Texture* GetDefaultPBRTexture()    { return tx_pbr; }

	}
}