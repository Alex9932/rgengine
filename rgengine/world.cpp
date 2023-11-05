#define DLL_EXPORT

#include "world.h"
#include "transform.h"
#include "entity.h"
#include "engine.h"

#define RG_ENTITY_COUNT 4096

namespace Engine {

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
		return ent;
	}

	void World::FreeEntity(Entity* e) {
		std::vector<Entity*>::iterator it = this->m_entities.begin();
		for (; it != this->m_entities.end(); it++) {
			if (*it = e) {
				this->m_entities.erase(it);
				m_allocEntity->Deallocate(e);
				break;
			}
		}
	}

	void World::Update() {

		Float64 dt = GetDeltaTime();

		std::vector<Entity*>::iterator it = this->m_entities.begin();
		for (; it != this->m_entities.end(); it++) {
			(*it)->Update(dt);
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

}