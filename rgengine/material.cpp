#include "material.h"
#include "allocator.h"

#include <map>

#define R_MAX_MATERIALS 16000

namespace Engine {
	namespace Render {

		static PoolAllocator* materialpool = NULL;
		static std::map<Uint64, R3D_Material*> materialcache;

		static R3D_Material* CreateMaterial(R3D_MaterialInfo* info) {
			R3D_Material* material = (R3D_Material*)materialpool->Allocate();

			material->albedo = NULL; // TODO: Load texture
			material->normal = NULL; // TODO: Load texture
			material->pbr    = NULL; // TODO: Load texture

			material->color = info->color;
			material->refcounter = 1;
			return material;
		}

		static void DestroyMaterial(R3D_Material* hmat) {
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

			// Return loaded material
			if (materialcache.count(hash) != 0) {
				R3D_Material* mat = materialcache[hash];
				mat->refcounter++;
				return mat;
			}

			// Create new material
			R3D_Material* material = CreateMaterial(info);
			materialcache[hash] = material;
			return material;
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