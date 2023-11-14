#ifndef _ENTITY_H
#define _ENTITY_H

#include "transform.h"
#include "uuid.h"

enum ComponentType {
	Component_TAG = 0,              // Name tag        TagComponent
	Component_MODELCOMPONENT,       // Model components
	Component_RIGGEDMODELCOMPONENT, //
	Component_POINTLIGHT,           // Light sources
	Component_SPOTLIGHT,            //
	Component_MAXENUM = 32
};

namespace Engine {

	class World;
	class Entity;
	class TagComponent;
	class ModelComponent;
	class RiggedModelComponent;
	class PointLight;
	class SpotLight;

	class Component {
		public:
			Component(ComponentType type) {
				this->m_ent  = NULL;
				this->m_type = type;
			}
			~Component() {}

			RG_INLINE void    SetEntity(Entity* ent) { this->m_ent = ent; }
			RG_INLINE Entity* GetEntity()            { return this->m_ent; }

			RG_INLINE ComponentType GetComponentType() { return this->m_type; }

			void Update(Float64 dt) {}

			RG_INLINE TagComponent* AsTagComponent() { return (TagComponent*)this; }
			RG_INLINE ModelComponent* AsModelComponent() { return (ModelComponent*)this; }
			RG_INLINE RiggedModelComponent* AsRiggedModelComponent() { return (RiggedModelComponent*)this; }

			RG_INLINE PointLight* AsPointLightComponent() { return (PointLight*)this; }
			RG_INLINE SpotLight* AsSpotLightComponent() { return (SpotLight*)this; }

        protected:
            //UUID          m_entID;
			Entity*	      m_ent;
            ComponentType m_type;

	};

	// Base components

	class TagComponent : public Component {
		private:
			char string[256];

		public:
			TagComponent(String tag) : Component(Component_TAG) {
				SDL_memset(this->string, 0, 256);
				SDL_strlcpy(this->string, tag, 256);
			}
			~TagComponent() {}
			RG_INLINE String GetString() { return this->string; }
			RG_INLINE void SetString(String tag) { SDL_strlcpy(this->string, tag, 256); }
	};


	class Entity {
		public:
			RG_DECLSPEC Entity(World* world);
			RG_DECLSPEC virtual ~Entity();

			RG_DECLSPEC void Update(Float64 dt);

			RG_DECLSPEC void AttachComponent(Component* comp);

			RG_INLINE void DetachComponent(ComponentType type) { this->m_components[type] = NULL; }
			RG_INLINE Component* GetComponent(ComponentType type) { return this->m_components[type]; }

			RG_INLINE UUID GetID() { return this->m_id; }
			RG_INLINE void SetID(UUID id) { this->m_id = id; } // !!! DO NOT CALL DIRECTLY !!!

			RG_INLINE Transform* GetTransform() { return this->m_transform; }

		private:
			UUID       m_id;
			Component* m_components[Component_MAXENUM];
			Transform* m_transform;
			World*     m_world;

	};

}

#endif