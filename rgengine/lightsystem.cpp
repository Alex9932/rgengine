#define DLL_EXPORT
#include "lightsystem.h"
#include "engine.h"
#include "render.h"
#include "world.h"

#define RG_LIGHTPOOL_SIZE 4096

namespace Engine {

	void PointLight::Destroy() {
		GetLightSystem()->DeletePointLight(this);
	}

	void SpotLight::Destroy() {
		GetLightSystem()->DeleteSpotLight(this);
	}

	LightSystem::LightSystem() {
		this->m_palloc = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Pointlight pool", RG_LIGHTPOOL_SIZE, sizeof(PointLight));
		this->m_salloc = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Spotlight pool", RG_LIGHTPOOL_SIZE, sizeof(SpotLight));
	}

	LightSystem::~LightSystem() {
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_palloc);
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_salloc);
	}

	void LightSystem::UpdateComponents() {

		Float64 dt = GetDeltaTime();

		std::vector<PointLight*>::iterator pit = this->m_pointlights.begin();
		for (; pit != this->m_pointlights.end(); pit++) {
			(*pit)->Update(dt);
		}

		std::vector<SpotLight*>::iterator sit = this->m_spotlights.begin();
		for (; sit != this->m_spotlights.end(); sit++) {
			(*sit)->Update(dt);
		}
	}

	PointLight* LightSystem::NewPointLight() {
		LightSource* src = GetWorld()->NewLightSource();
		src->source.type = RG_POINTLIGHT;
		PointLight* comp = RG_NEW_CLASS(this->m_palloc, PointLight)(src);
		this->m_pointlights.push_back(comp);
		return comp;
	}

	void LightSystem::DeletePointLight(PointLight* comp) {
		std::vector<PointLight*>::iterator it = this->m_pointlights.begin();
		for (; it != this->m_pointlights.end(); it++) {
			if (*it == comp) {
				*it = std::move(m_pointlights.back());
				m_pointlights.pop_back();
				//this->m_pointlights.erase(it);
				RG_DELETE_CLASS(this->m_palloc, PointLight, comp);
				break;
			}
		}
	}

	SpotLight* LightSystem::NewSpotLight() {
		LightSource* src = GetWorld()->NewLightSource();
		src->source.type = RG_SPOTLIGHT;
		SpotLight* comp = RG_NEW_CLASS(this->m_salloc, SpotLight)(src);
		this->m_spotlights.push_back(comp);
		return comp;
	}

	void LightSystem::DeleteSpotLight(SpotLight* comp) {
		std::vector<SpotLight*>::iterator it = this->m_spotlights.begin();
		for (; it != this->m_spotlights.end(); it++) {
			if (*it == comp) {
				*it = std::move(m_spotlights.back());
				m_spotlights.pop_back();
				//this->m_spotlights.erase(it);
				RG_DELETE_CLASS(this->m_salloc, SpotLight, comp);
				break;
			}
		}
	}

}