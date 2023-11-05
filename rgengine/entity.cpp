#include "entity.h"
#include "world.h"

namespace Engine {


	Entity::Entity(World* world) {
		this->m_world = world;
		this->m_id = GenerateUUID();
		SDL_memset(this->m_components, 0, sizeof(Component*) * Component_MAXENUM);
		this->m_transform = world->NewTransform();
		//SDL_memset(this->m_transform, 0, sizeof(Transform));
	}

	Entity::~Entity() {
		this->m_world->FreeTransform(this->m_transform);
	}

	void Entity::Update(double dt) {
		this->m_transform->Recalculate();
	}

	void Entity::AttachComponent(Component* comp) {
		comp->SetEntity(this);
		this->m_components[comp->GetComponentType()] = comp;
	}

}