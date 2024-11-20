#ifndef _WORLD_H
#define _WORLD_H

#include "rgtypes.h"
#include "allocator.h"
#include "uuid.h"

namespace Engine {

	class Transform;
	class Entity;
	class StaticObject;

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

			RG_DECLSPEC StaticObject* GetStaticObject(Uint32 idx);
			RG_INLINE   Uint32        GetStaticCount() { return (Uint32)m_static.size(); }

			RG_DECLSPEC void ClearWorld();

		private:
			PoolAllocator* m_allocTransform;
			PoolAllocator* m_allocEntity;
			PoolAllocator* m_allocStatic;

			// TODO: Optimize this
			std::vector<Entity*> m_entities;
			std::vector<Entity*> m_delqueue;

			std::vector<StaticObject*> m_static;

	};

}

#endif