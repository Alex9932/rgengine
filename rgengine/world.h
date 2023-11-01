#ifndef _WORLD_H
#define _WORLD_H

#define RG_TRANSFORM_COUNT 4096

#include "rgtypes.h"
#include "allocator.h"

namespace Engine {

	class Transform;

	class World {
		public:
			RG_DECLSPEC World();
			RG_DECLSPEC ~World();

			RG_DECLSPEC Transform* NewTransform();
			RG_DECLSPEC void FreeTransform(Transform* t);

		private:
			PoolAllocator* m_allocTransform;


	};

}

#endif