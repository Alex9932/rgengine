#define DLL_EXPORT

#include "world.h"
#include "transform.h"

namespace Engine {

	World::World() {
		m_allocTransform = new PoolAllocator("Transform pool", RG_TRANSFORM_COUNT, sizeof(Transform));
	}

	World::~World() {
		delete m_allocTransform;
	}

	Transform* World::NewTransform() {
		return RG_NEW_CLASS(m_allocTransform, Transform)();
	}

	void World::FreeTransform(Transform* t) {
		m_allocTransform->Deallocate(t);
	}

}