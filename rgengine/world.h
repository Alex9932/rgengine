#ifndef _WORLD_H
#define _WORLD_H

#include "rgtypes.h"
#include "allocator.h"
#include "uuid.h"

namespace Engine {

	class Transform;
	class Entity;

	class World {
		public:
			RG_DECLSPEC World();
			RG_DECLSPEC ~World();

			RG_DECLSPEC Transform* NewTransform();
			RG_DECLSPEC void FreeTransform(Transform* t);

			RG_DECLSPEC Entity* NewEntity();
			RG_DECLSPEC void FreeEntity(Entity* e);

			RG_DECLSPEC void Update();

			RG_DECLSPEC Entity* GetEntity(Uint32 idx);
			RG_DECLSPEC Entity* GetEntityByUUID(UUID uuid);
			RG_INLINE   Uint32  GetEntityCount() { return (Uint32)m_entities.size(); }

			RG_DECLSPEC void ClearWorld();

		private:
			PoolAllocator* m_allocTransform;
			PoolAllocator* m_allocEntity;

			// TODO: Optimize this
			std::vector<Entity*> m_entities;

	};

}

#endif