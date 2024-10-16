#define DLL_EXPORT

#include "world.h"
#include "transform.h"
#include "entity.h"
#include "engine.h"

#include "render.h"
#include "modelsystem.h"
#include "lightsystem.h"

#define RG_ENTITY_COUNT 4096

namespace Engine {

	static void FreeComponents(Entity* ent) {
		for (Uint32 i = 0; i < Component_MAXENUM; i++) {
			Component* c = ent->GetComponent((ComponentType)i);
			if (c) { c->Destroy(); }
		}
	}

	World::World() {
		this->m_allocTransform = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Transform pool", RG_ENTITY_COUNT, sizeof(Transform));
		this->m_allocEntity    = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Entity pool",    RG_ENTITY_COUNT, sizeof(Entity));
	}

	World::~World() {
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_allocTransform);
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_allocEntity);
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
		m_delqueue.push_back(e);
	}

	static void RemoveEntity(Allocator* alloc, std::vector<Entity*>& entities, Entity* ent_ptr) {
		std::vector<Entity*>::iterator it = entities.begin();
		Uint32 idx = 0;
		for (; it != entities.end(); it++) {
			if (*it == ent_ptr) {

				*it = std::move(entities.back());
				entities.pop_back();

				FreeComponents(ent_ptr);
				RG_DELETE_CLASS(alloc, Entity, ent_ptr);
				break;
			}
			idx++;
		}
	}

	void World::Update() {

		Float64 dt = GetDeltaTime();

		// Remove entities
		for (size_t i = 0; i < m_delqueue.size(); i++) {
			RemoveEntity(m_allocEntity, m_entities, m_delqueue[i]);
		}
		m_delqueue.clear();

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

	Entity* World::GetEntityByUUID(UUID uuid) {
		std::vector<Entity*>::iterator it = this->m_entities.begin();
		for (; it != this->m_entities.end(); it++) {
			Entity* e = *it;
			if (e->GetID() == uuid) {
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

		//m_allocTransform->DeallocateAll();
		m_allocEntity->DeallocateAll();
		m_entities.clear();
	}

}