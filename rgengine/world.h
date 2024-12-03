#ifndef _WORLD_H
#define _WORLD_H

#include "rgtypes.h"
#include "allocator.h"
#include "uuid.h"

#include "rgmatrix.h"

struct R3D_StaticModel;
struct AABB;
struct LightSource;

namespace Engine {

	class Transform;
	class Entity;
	class StaticObject;

	class World {
		public:
			RG_DECLSPEC World();
			RG_DECLSPEC ~World();

			RG_DECLSPEC void Update();

			RG_DECLSPEC Transform* NewTransform();
			RG_DECLSPEC void FreeTransform(Transform* t);

			RG_DECLSPEC Entity* NewEntity();
			RG_DECLSPEC void FreeEntity(Entity* e);

			RG_DECLSPEC Entity* GetEntity(Uint32 idx);
			RG_DECLSPEC Entity* GetEntityByUUID(RGUUID uuid);
			RG_INLINE   Uint32  GetEntityCount() { return (Uint32)m_entities.size(); }

			RG_DECLSPEC StaticObject* NewStatic(R3D_StaticModel* handle, mat4* transform, AABB* aabb);
			RG_DECLSPEC void FreeStatic(StaticObject* s);

			RG_DECLSPEC StaticObject* GetStaticObject(Uint32 idx);
			RG_DECLSPEC StaticObject* GetStaticObjectByUUID(RGUUID uuid);
			RG_INLINE   Uint32        GetStaticCount() { return (Uint32)m_static.size(); }

			RG_DECLSPEC LightSource* NewLightSource();
			RG_DECLSPEC void FreeStatic(LightSource* src);

			RG_DECLSPEC LightSource* GetLightSource(Uint32 idx);
			RG_DECLSPEC LightSource* GetLightSourceByUUID(RGUUID uuid);
			RG_INLINE   Uint32       GetLightCount() { return (Uint32)m_entities.size(); }



			RG_DECLSPEC void ClearWorld();

		private:
			PoolAllocator* m_allocTransform;
			PoolAllocator* m_allocEntity;
			PoolAllocator* m_allocStatic;
			PoolAllocator* m_allocLight;

			
			std::vector<Entity*>       m_entities;
			std::vector<StaticObject*> m_static;
			std::vector<LightSource*>  m_lights;

			// TODO: Optimize this
			std::vector<Entity*>       m_entitydelqueue;
			std::vector<StaticObject*> m_staticdelqueue;
			std::vector<LightSource*>  m_lightsdelqueue;

	};

}

#endif