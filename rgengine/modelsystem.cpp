#define DLL_EXPORT
#include "modelsystem.h"
#include "engine.h"
#include "render.h"
#include "kinematicsmodel.h"
#include "rmodelmanager.h"

#define RG_MODELPOOL_SIZE 4096

namespace Engine {

	ModelComponent::ModelComponent(String model) : Component(Component_MODELCOMPONENT) {
		this->m_handle = GetStaticModel(model);
	}

	ModelComponent::~ModelComponent() {
		FreeStaticModel(this->m_handle);
		//Render::DestroyStaticModel(GetHandle());
		//GetModelSystem()->DeleteModelComponent(this);
	}

	//void ModelComponent::Destroy() { }

	RiggedModelComponent::RiggedModelComponent(String kmdl) : Component(Component_RIGGEDMODELCOMPONENT) {
		this->m_handle = GetRiggedModel(kmdl);
		this->m_kmodel = GetUniqueKinematicsModel(this->m_handle);

		R3DCreateBufferInfo binfo = {};
		binfo.len = sizeof(mat4) * this->m_kmodel->GetBoneCount();
		binfo.initialData = NULL;
		this->m_bonebuffer = Render::CreateBoneBuffer(&binfo);
	}

	RiggedModelComponent::~RiggedModelComponent() {
		Render::DestroyBoneBuffer(this->m_bonebuffer);
		FreeUniqueKinematicsModel(this->m_kmodel);
		FreeRiggedModel(this->m_handle);
		//RG_DELETE_CLASS(GetDefaultAllocator(), KinematicsModel, m_kmodel);
		//Render::DestroyRiggedModel(GetHandle());

		//GetModelSystem()->DeleteRiggedModelComponent(this);
	}

	//void RiggedModelComponent::Destroy() { }


	ModelSystem::ModelSystem() {
		this->m_alloc  = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Model pool", RG_MODELPOOL_SIZE, sizeof(ModelComponent));
		this->m_ralloc = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("RModel pool", RG_MODELPOOL_SIZE, sizeof(RiggedModelComponent));
	}

	ModelSystem::~ModelSystem() {
		while (m_modelComponents.size() > 0) {
			UpdateComponents();
		}
		while (m_rmodelComponents.size() > 0) {
			UpdateComponents();
		}

		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_alloc);
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, this->m_ralloc);
	}

	void ModelSystem::UpdateComponents() {

		Float64 dt = GetDeltaTime();


		// Pointers to components that should be deleted after loop, to avoid invalidating iterators
		ModelComponent*       delete_mc  = NULL;
		RiggedModelComponent* delete_rmc = NULL;

		std::vector<ModelComponent*>::iterator it = this->m_modelComponents.begin();
		for (; it != this->m_modelComponents.end(); it++) {
			ModelComponent* mc = *it;
			if (mc->GetEntity() == NULL) {
				// Detached component, delete it
				// THIS DELETE ONE COMPONENT PER FRAME!
				delete_mc = mc;
				continue;
			}
			mc->Update(dt);
		}

		std::vector<RiggedModelComponent*>::iterator rit = this->m_rmodelComponents.begin();
		for (; rit != this->m_rmodelComponents.end(); rit++) {
			RiggedModelComponent* rmc = *rit;
			if (rmc->GetEntity() == NULL) {
				// Same as for model components
				delete_rmc = rmc;
				continue;
			}
			rmc->Update(dt);
		}

		// Delete marked components
		if (delete_mc) {
			DeleteModelComponent(delete_mc);
		}
		if (delete_rmc) {
			DeleteRiggedModelComponent(delete_rmc);
		}
	}

	ModelComponent* ModelSystem::NewModelComponent(String model) {
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

	RiggedModelComponent* ModelSystem::NewRiggedModelComponent(String model) {
		RiggedModelComponent* comp = RG_NEW_CLASS(this->m_ralloc, RiggedModelComponent)(model);
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