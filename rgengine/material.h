#ifndef _MATERIAL_H
#define _MATERIAL_H

#include "rendertypes.h"

struct Texture;

typedef struct R3D_Material {
	Texture* albedo;
	Texture* normal;
	Texture* pbr;
	vec3     color;
	Uint32   refcounter;
} R3D_Material;

namespace Engine {
	namespace Render {

		void InitializeMaterials();
		void DestroyMaterials();
		R3D_Material* GetMaterial(R3D_MaterialInfo* info);
		void FreeMaterial(R3D_Material* hmat);

	}
}

#endif