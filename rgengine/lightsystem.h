#ifndef _LIGHTSYSTEM_H
#define _LIGHTSYSTEM_H

#include "entity.h"
#include "rgvector.h"
#include "allocator.h"

#include "rendertypes.h"

#include <vector>

typedef struct LightSource {
	RGUUID uuid;
	R3D_LightSource source;
	// Other info (for future)
} LightSource;

namespace Engine {

	class PointLight : public Component {
		public:
			PointLight(LightSource* src) : Component(Component_POINTLIGHT) { m_source = src; }
			~PointLight() {}

			virtual void Destroy();

			RG_INLINE void Update(Float64 dt) {
				if (m_ent) {
					//this->m_position = this->m_ent->GetTransform()->GetWorldPosition() + this->m_offset;
					m_source->source.position = m_ent->GetTransform()->GetWorldPosition();
				}
			}

			RG_INLINE void    SetColor(const vec3& color)  { m_source->source.color = color; }
			RG_INLINE vec3&   GetColor()                   { return m_source->source.color;  }

			RG_INLINE void    SetIntensity(Float32 intens) { m_source->source.intensity = intens; }
			RG_INLINE Float32 GetIntensity()               { return m_source->source.intensity;   }

			RG_INLINE void    SetPosition(const vec3& pos) { m_source->source.position = pos;  }
			RG_INLINE vec3&   GetPosition()                { return m_source->source.position; }

			//RG_INLINE void    SetOffset(const vec3& off)   { this->m_source.offset = off; }

		private:
			LightSource* m_source;
	};

	class SpotLight : public Component {
		public:
			SpotLight(LightSource* src) : Component(Component_SPOTLIGHT) { m_source = src; }
			~SpotLight() {}

			virtual void Destroy();

			RG_INLINE void Update(Float64 dt) {
				if (this->m_ent) {
					//this->m_position = this->m_ent->GetTransform()->GetWorldPosition() + this->m_offset;
					m_source->source.position = this->m_ent->GetTransform()->GetWorldPosition();
				}
			}

			RG_INLINE void    SetColor(const vec3& color)  { m_source->source.color = color; }
			RG_INLINE vec3&   GetColor()                   { return m_source->source.color;  }

			RG_INLINE void    SetIntensity(Float32 intens) { m_source->source.intensity = intens; }
			RG_INLINE Float32 GetIntensity()               { return m_source->source.intensity;   }

			RG_INLINE void    SetPosition(const vec3& pos) { m_source->source.position = pos;  }
			RG_INLINE vec3&   GetPosition()                { return m_source->source.position; }

			RG_INLINE void    SetDirection(const vec3& dir) { m_source->source.direction = dir;  }
			RG_INLINE vec3&   GetDirection()                { return m_source->source.direction; }

			RG_INLINE void    SetInnerCone(Float32 factor) { m_source->source.innerCone = factor; }
			RG_INLINE Float32 GetInnerCone()               { return m_source->source.innerCone; }

			RG_INLINE void    SetOuterCone(Float32 factor) { m_source->source.outerCone = factor; }
			RG_INLINE Float32 GetOuterCone()               { return m_source->source.outerCone; }

			//RG_INLINE void    SetOffset(const vec3& off)    { this->m_offset = off;  }
			//RG_INLINE vec3&   GetOffset()                   { return this->m_offset; }

		private:
			LightSource* m_source;
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