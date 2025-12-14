#include "material.h"
#include "allocator.h"
#include "texture.h"
#include "filesystem.h"

#include <map>

#define R_MAX_MATERIALS 16000

namespace Engine {
	namespace Render {

		static PoolAllocator* materialpool = NULL;
		static std::map<Uint64, R3D_Material*> materialcache;

		static R3D_Material* CreateMaterial(R3D_MaterialInfo* info) {
			R3D_Material* material = (R3D_Material*)materialpool->Allocate();

			char albedo[256];
			char normal[256];
			char pbr[256];
			SDL_snprintf(albedo, 256, "%s/textures/%s.png",      GetGamedataPath(), info->texture);
			SDL_snprintf(normal, 256, "%s/textures/%s_norm.png", GetGamedataPath(), info->texture);
			SDL_snprintf(pbr,    256, "%s/textures/%s_pbr.png",  GetGamedataPath(), info->texture);

			if (!FS_IsExist(albedo)) {
				rgLogWarn(RG_LOG_RENDER, "Texture (%s) not found! Using default", albedo);
				SDL_snprintf(albedo, 256, "platform/textures/def_diffuse.png", GetGamedataPath());
			}
			if (!FS_IsExist(normal)) {
				SDL_snprintf(normal, 256, "platform/textures/def_normal.png", GetGamedataPath());
			}
			if (!FS_IsExist(pbr)) {
				SDL_snprintf(pbr,    256, "platform/textures/def_pbr.png", GetGamedataPath());
			}

			// NEED FOR ~SPIRT~ TEXTURES
			material->albedo = GetTexture(albedo);
			material->normal = GetTexture(normal);
			material->pbr    = GetTexture(pbr);

			material->color = info->color;
			material->refcounter = 1;
			return material;
		}

		static void DestroyMaterial(R3D_Material* hmat) {
			FreeTexture(hmat->albedo);
			FreeTexture(hmat->normal);
			FreeTexture(hmat->pbr);
			materialpool->Deallocate(hmat);
		}

		void InitializeMaterials() {
			materialcache.clear();
			materialpool = RG_NEW(PoolAllocator)("Material Pool Allocator", R_MAX_MATERIALS, sizeof(R3D_Material));
		}

		void DestroyMaterials() {
			RG_DELETE(PoolAllocator, materialpool);
		}

		R3D_Material* GetMaterial(R3D_MaterialInfo* info) {
			Uint64 hash = rgHash(info, sizeof(R3D_MaterialInfo));
			R3D_Material* mat = NULL;

			// Return loaded material
			if (materialcache.count(hash) != 0) {
				mat = materialcache[hash];
				mat->refcounter++;
				return mat;
			}

			// Create new material
			mat = CreateMaterial(info);
			materialcache[hash] = mat;
			return mat;
		}

		void FreeMaterial(R3D_Material* hmat) {
			hmat->refcounter--;
			if (hmat->refcounter == 0) {
				for (auto it = materialcache.begin(); it != materialcache.end(); ) {
					if (it->second == hmat) {
						materialcache.erase(it++);
					} else {
						++it;
					}
				}
				DestroyMaterial(hmat);
			}
		}

	}
}