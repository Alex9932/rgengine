#ifndef _LIGHTSYSTEM_H
#define _LIGHTSYSTEM_H

#include "entity.h"
#include "rgvector.h"
#include "allocator.h"

#include "rendertypes.h"

#include <vector>

namespace Engine {

	class PointLight : public Component {
		public:
			PointLight() : Component(Component_POINTLIGHT) {
				m_source = {};
				m_source.uuid = GenerateUUID();
				m_source.type = RG_POINTLIGHT;
			}
			~PointLight() {}

			virtual void Destroy();

			RG_INLINE void Update(Float64 dt) {
				if (this->m_ent) {
					//this->m_position = this->m_ent->GetTransform()->GetWorldPosition() + this->m_offset;
					this->m_source.position = this->m_ent->GetTransform()->GetWorldPosition();
				}
			}

			RG_INLINE void    SetColor(const vec3& color)  { this->m_source.color = color; }
			RG_INLINE vec3&   GetColor()                   { return this->m_source.color;  }

			RG_INLINE void    SetIntensity(Float32 intens) { this->m_source.intensity = intens; }
			RG_INLINE Float32 GetIntensity()               { return this->m_source.intensity;   }

			RG_INLINE void    SetPosition(const vec3& pos) { this->m_source.position = pos;  }
			RG_INLINE vec3&   GetPosition()                { return this->m_source.position; }

			//RG_INLINE void    SetOffset(const vec3& off)   { this->m_source.offset = off; }

		private:
			R3D_LightSource m_source;
	};

	class SpotLight : public Component {
		public:
			SpotLight() : Component(Component_SPOTLIGHT) {
				m_source = {};
				m_source.uuid = GenerateUUID();
				m_source.type = RG_SPOTLIGHT;
			}
			~SpotLight() {}

			virtual void Destroy();

			RG_INLINE void Update(Float64 dt) {
				if (this->m_ent) {
					//this->m_position = this->m_ent->GetTransform()->GetWorldPosition() + this->m_offset;
					this->m_source.position = this->m_ent->GetTransform()->GetWorldPosition();
				}
			}

			RG_INLINE void    SetColor(const vec3& color) { this->m_source.color = color; }
			RG_INLINE vec3&   GetColor()                  { return this->m_source.color;  }

			RG_INLINE void    SetIntensity(Float32 intens) { this->m_source.intensity = intens; }
			RG_INLINE Float32 GetIntensity()               { return this->m_source.intensity;   }

			RG_INLINE void    SetPosition(const vec3& pos) { this->m_source.position = pos;  }
			RG_INLINE vec3&   GetPosition()                { return this->m_source.position; }

			RG_INLINE void    SetDirection(const vec3& dir) { this->m_source.direction = dir;  }
			RG_INLINE vec3&   GetDirection()                { return this->m_source.direction; }

			RG_INLINE void    SetCutoff(Float32 factor) { this->m_source.coneAngle = factor; }
			RG_INLINE Float32 GetCutoff()               { return this->m_source.coneAngle;   }

			//RG_INLINE void    SetOffset(const vec3& off)    { this->m_offset = off;  }
			//RG_INLINE vec3&   GetOffset()                   { return this->m_offset; }

		private:
			R3D_LightSource m_source;
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

			RG_INLINE Uint32 GetPointLightCount() { return (Uint32)m_pointlights.size(); }
			RG_INLINE Uint32 GetSpotLightCount()  { return (Uint32)m_spotlights.size(); }

		private:

			Engine::PoolAllocator* m_palloc;
			Engine::PoolAllocator* m_salloc;

			std::vector<PointLight*> m_pointlights;
			std::vector<SpotLight*>  m_spotlights;

	};

}

#endif