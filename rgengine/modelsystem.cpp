#define DLL_EXPORT
#include "modelsystem.h"
#include "engine.h"
#include "render.h"
#include "kinematicsmodel.h"

#define RG_MODELPOOL_SIZE 4096

namespace Engine {

	ModelComponent::ModelComponent(R3D_StaticModel* model) : Component(Component_MODELCOMPONENT) {
		this->m_handle = model;
	}

	ModelComponent::~ModelComponent() { }

	void ModelComponent::Destroy() {
		Render::DestroyStaticModel(GetHandle());
		GetModelSystem()->DeleteModelComponent(this);
	}

	RiggedModelComponent::RiggedModelComponent(R3D_RiggedModel* rmdl, KinematicsModel* kmdl) : Component(Component_RIGGEDMODELCOMPONENT) {
		this->m_handle = rmdl;
		this->m_kmodel = kmdl;
	}

	RiggedModelComponent::~RiggedModelComponent() { }

	void RiggedModelComponent::Destroy() {
		Render::DestroyBoneBuffer(m_kmodel->GetBufferHandle());
		RG_DELETE_CLASS(GetDefaultAllocator(), KinematicsModel, m_kmodel);
		Render::DestroyRiggedModel(GetHandle());
		GetModelSystem()->DeleteRiggedModelComponent(this);
	}


	ModelSystem::ModelSystem() {
		this->m_alloc  = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Model pool", RG_MODELPOOL_SIZE, sizeof(ModelComponent));
		this->m_ralloc = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("RModel pool", RG_MODELPOOL_SIZE, sizeof(RiggedModelComponent));
	}

	ModelSystem::~ModelSystem() {
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_alloc);
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_ralloc);
	}

	void ModelSystem::UpdateComponents() {

		Float64 dt = GetDeltaTime();

		std::vector<ModelComponent*>::iterator it = this->m_modelComponents.begin();
		for (; it != this->m_modelComponents.end(); it++) {
			(*it)->Update(dt);
		}

		std::vector<RiggedModelComponent*>::iterator rit = this->m_rmodelComponents.begin();
		for (; rit != this->m_rmodelComponents.end(); rit++) {
			(*rit)->Update(dt);
		}
	}

	ModelComponent* ModelSystem::NewModelComponent(R3D_StaticModel* model) {
		ModelComponent* comp = RG_NEW_CLASS(this->m_alloc, ModelComponent)(model);
		this->m_modelComponents.push_back(comp);
		return comp;
	}

	void ModelSystem::DeleteModelComponent(ModelComponent* comp) {
		std::vector<ModelComponent*>::iterator it = this->m_modelComponents.begin();
		for (; it != this->m_modelComponents.end(); it++) {
			if(*it == comp) {
				*it = std::move(m_modelComponents.back());
				m_modelComponents.pop_back();
				//this->m_modelComponents.erase(it);
				RG_DELETE_CLASS(this->m_alloc, ModelComponent, comp);
				break;
			}
		}
	}

	RiggedModelComponent* ModelSystem::NewRiggedModelComponent(R3D_RiggedModel* model, KinematicsModel* kmodel) {
		RiggedModelComponent* comp = RG_NEW_CLASS(this->m_ralloc, RiggedModelComponent)(model, kmodel);
		this->m_rmodelComponents.push_back(comp);
		return comp;
	}

	void ModelSystem::DeleteRiggedModelComponent(RiggedModelComponent* comp) {
		std::vector<RiggedModelComponent*>::iterator it = this->m_rmodelComponents.begin();
		for (; it != this->m_rmodelComponents.end(); it++) {
			if (*it == comp) {
				*it = std::move(m_rmodelComponents.back());
				m_rmodelComponents.pop_back();
				//this->m_rmodelComponents.erase(it);
				RG_DELETE_CLASS(this->m_ralloc, RiggedModelComponent, comp);
				break;
			}
		}
	}

}