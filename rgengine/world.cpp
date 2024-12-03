#define DLL_EXPORT

#include "world.h"
#include "transform.h"
#include "entity.h"
#include "staticobject.h"
#include "engine.h"

#include "render.h"
#include "modelsystem.h"
#include "lightsystem.h"

// HARD-LOCKED COUNTS
#define RG_ENTITY_COUNT 4096
#define RG_STATIC_COUNT 16384
#define RG_LIGHTS_COUNT 1024

namespace Engine {

	static void FreeComponents(Entity* ent) {
		for (Uint32 i = 0; i < Component_MAXENUM; i++) {
			Component* c = ent->GetComponent((ComponentType)i);
			if (c) { c->Destroy(); }
		}
	}

	World::World() {
		this->m_allocTransform = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Transform pool",  RG_ENTITY_COUNT, sizeof(Transform));
		this->m_allocEntity    = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Entity pool",     RG_ENTITY_COUNT, sizeof(Entity));
		this->m_allocStatic    = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Static geometry", RG_STATIC_COUNT, sizeof(StaticObject));
		this->m_allocLight     = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Light sources",   RG_LIGHTS_COUNT, sizeof(LightSource));
	}

	World::~World() {
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_allocTransform);
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_allocEntity);
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_allocStatic);
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_allocLight);
	}

	Transform* World::NewTransform() {
		return RG_NEW_CLASS(m_allocTransform, Transform)();
	}

	void World::FreeTransform(Transform* t) {
		m_allocTransform->Deallocate(t);
	}

	Entity* World::NewEntity() {
		Entity* ent = RG_NEW_CLASS(m_allocEntity, Entity)(this);
		this->m_entities.push_back(ent);
		ent->AttachComponent(RG_NEW(TagComponent)("new entity"));
		return ent;
	}

	void World::FreeEntity(Entity* e) {
		// Add to deletion queue
		m_entitydelqueue.push_back(e);
	}
	/*
	template <typename T>
	static void RemoveObject(Allocator* alloc, std::vector<T*>& objects, T* ptr) {
		std::vector<T*>::iterator it = objects.begin();
		Uint32 idx = 0;
		for (; it != objects.end(); it++) {
			if (*it == ptr) {

				*it = std::move(objects.back());
				objects.pop_back();

				RG_DELETE_CLASS(alloc, T, ptr);
				break;
			}
			idx++;
		}
	}
	*/

	static void RemoveEntity(Allocator* alloc, std::vector<Entity*>& objects, Entity* s_ptr) {
		std::vector<Entity*>::iterator it = objects.begin();
		Uint32 idx = 0;

		for (; it != objects.end(); it++) {
			if (*it == s_ptr) {

				*it = std::move(objects.back());
				objects.pop_back();

				RG_DELETE_CLASS(alloc, Entity, s_ptr);
				break;
			}
			idx++;
		}
	}

	static void RemoveStatic(Allocator* alloc, std::vector<StaticObject*>& objects, StaticObject* s_ptr) {
		std::vector<StaticObject*>::iterator it = objects.begin();
		Uint32 idx = 0;

		for (; it != objects.end(); it++) {
			if (*it == s_ptr) {

				*it = std::move(objects.back());
				objects.pop_back();

				RG_DELETE_CLASS(alloc, StaticObject, s_ptr);
				break;
			}
			idx++;
		}
	}

	void World::Update() {

		Float64 dt = GetDeltaTime();

		// Remove entities
		for (size_t i = 0; i < m_entitydelqueue.size(); i++) {
			FreeComponents(m_entitydelqueue[i]);
			RemoveEntity(m_allocEntity, m_entities, m_entitydelqueue[i]);
		}
		m_entitydelqueue.clear();

		// Remove static
		for (size_t i = 0; i < m_staticdelqueue.size(); i++) {
			RemoveStatic(m_allocStatic, m_static, m_staticdelqueue[i]);
		}
		m_staticdelqueue.clear();

		// Update

		std::vector<Entity*>::iterator it = this->m_entities.begin();

		Entity* ent = NULL;
		EntityBehavior* ent_behavior = NULL;
		for (; it != this->m_entities.end(); it++) {
			ent = (*it);
			ent_behavior = ent->GetBehavior();
			if (ent_behavior) {
				ent_behavior->Update(dt);
			}
			ent->Update(dt);
		}

	}

	Entity* World::GetEntity(Uint32 idx) {
		return this->m_entities[idx];
	}

	Entity* World::GetEntityByUUID(RGUUID uuid) {
		std::vector<Entity*>::iterator it = this->m_entities.begin();
		for (; it != this->m_entities.end(); it++) {
			Entity* e = *it;
			if (e->GetID() == uuid) {
				return e;
			}
		}
		return NULL;
	}

	StaticObject* World::NewStatic(R3D_StaticModel* h, mat4* t, AABB* aabb) {
		StaticObject* s = RG_NEW_CLASS(m_allocStatic, StaticObject)(h, t, aabb);
		this->m_static.push_back(s);
		return s;
	}

	void World::FreeStatic(StaticObject* s) {
		m_staticdelqueue.push_back(s);
	}

	StaticObject* World::GetStaticObject(Uint32 idx) {
		return this->m_static[idx];
	}

	StaticObject* World::GetStaticObjectByUUID(RGUUID uuid) {
		std::vector<StaticObject*>::iterator it = this->m_static.begin();
		for (; it != this->m_static.end(); it++) {
			StaticObject* e = *it;
			if (e->GetID() == uuid) {
				return e;
			}
		}
		return NULL;
	}

	LightSource* World::NewLightSource() {
		LightSource* s = (LightSource*)m_allocLight->Allocate();
		s->uuid = GenerateUUID();
		this->m_lights.push_back(s);
		return s;
	}

	void World::FreeLightSource(LightSource* src) {
		m_lightsdelqueue.push_back(src);
	}

	LightSource* World::GetLightSource(Uint32 idx) {
		return this->m_lights[idx];
	}

	LightSource* World::GetLightSourceByUUID(RGUUID uuid) {
		std::vector<LightSource*>::iterator it = this->m_lights.begin();
		for (; it != this->m_lights.end(); it++) {
			LightSource* e = *it;
			if (e->uuid == uuid) {
				return e;
			}
		}
		return NULL;
	}

	void World::ClearWorld() {

		for (Uint32 i = 0; i < m_entities.size(); i++) {
			Entity* ent = m_entities[i];
			
			FreeComponents(ent);

			RG_DELETE_CLASS(m_allocEntity, Entity, ent);
		}

		// WARN: Destructor NOT CALLED !!!
		m_allocEntity->DeallocateAll();
		m_allocTransform->DeallocateAll();
		m_entities.clear();
	}

}