#ifndef _LIGHTSYSTEM_H
#define _LIGHTSYSTEM_H

#include "entity.h"
#include "rgvector.h"
#include "allocator.h"

#include <vector>

namespace Engine {

	class PointLight : public Component {
		public:
			PointLight()  : Component(Component_POINTLIGHT) {}
			~PointLight() {}

			RG_INLINE void Update(Float64 dt) {
				if (this->m_ent) {
					this->m_position = this->m_ent->GetTransform()->GetWorldPosition() + this->m_offset;
				}
			}

			RG_INLINE void    SetColor(const vec3& color)  { this->m_color = color; }
			RG_INLINE vec3&   GetColor()                   { return this->m_color;  }

			RG_INLINE void    SetIntensity(Float32 intens) { this->m_intensity = intens; }
			RG_INLINE Float32 GetIntensity()               { return this->m_intensity;   }

		private:
			vec3    m_offset;
			vec3    m_position;
			Float32 m_intensity;
			vec3    m_color;
	};

	class SpotLight : public Component {
		public:
			SpotLight() : Component(Component_SPOTLIGHT) {}
			~SpotLight() {}

			RG_INLINE void Update(Float64 dt) {
				if (this->m_ent) {
					this->m_position = this->m_ent->GetTransform()->GetWorldPosition() + this->m_offset;
				}
			}

			RG_INLINE void    SetOffset(const vec3& off)    { this->m_offset = off;  }
			RG_INLINE vec3&   GetOffset()                   { return this->m_offset; }

			RG_INLINE void    SetDirection(const vec3& dir) { this->m_direction = dir;  }
			RG_INLINE vec3&   GetDirection()                { return this->m_direction; }
			
			RG_INLINE void    SetColor(const vec3& color)   { this->m_color = color; }
			RG_INLINE vec3&   GetColor()                    { return this->m_color;  }

			RG_INLINE void    SetIntensity(Float32 intens)  { this->m_intensity = intens; }
			RG_INLINE Float32 GetIntensity()                { return this->m_intensity;   }

		private:
			vec3    m_offset;
			vec3    m_position;
			Float32 m_intensity;
			vec3    m_color;
			vec3    m_direction;
	};

	class LightSystem {
		public:
			LightSystem();
			~LightSystem();

			RG_DECLSPEC void UpdateComponents();

			RG_DECLSPEC PointLight* NewPointLight();
			RG_DECLSPEC void DeletePointLight(PointLight* comp);

			RG_DECLSPEC SpotLight* NewSpotLight();
			RG_DECLSPEC void DeleteSpotLight(SpotLight* comp);

			RG_INLINE PointLight* GetPointLight(Uint32 idx) { return m_pointlights[idx]; }
			RG_INLINE SpotLight*  GetSpotLight(Uint32 idx)  { return m_spotlights[idx]; }

			RG_INLINE Uint32 GetPointLightCount() { return m_pointlights.size(); }
			RG_INLINE Uint32 GetSpotLightCount()  { return m_spotlights.size(); }

		private:

			Engine::PoolAllocator* m_palloc;
			Engine::PoolAllocator* m_salloc;

			std::vector<PointLight*> m_pointlights;
			std::vector<SpotLight*>  m_spotlights;

	};

}

#endif