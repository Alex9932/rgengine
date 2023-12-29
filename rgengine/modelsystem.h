#ifndef _MODELSYSTEM_H
#define _MODELSYSTEM_H

#include "allocator.h"
#include "rendertypes.h"
#include "entity.h"
#include <vector>

namespace Engine {

	class KinematicsModel;

	class ModelComponent : public Component {
		public:
			ModelComponent(R3D_StaticModel* model);
			~ModelComponent();
			virtual void Destroy();

			RG_INLINE R3D_StaticModel* GetHandle() { return this->m_handle; }

		private:
			R3D_StaticModel* m_handle;
	};

	class RiggedModelComponent : public Component {
		public:
			RiggedModelComponent(R3D_RiggedModel* rmdl, KinematicsModel* kmdl);
			~RiggedModelComponent();
			virtual void Destroy();

			RG_INLINE R3D_RiggedModel* GetHandle()			{ return this->m_handle; }
			RG_INLINE KinematicsModel* GetKinematicsModel() { return this->m_kmodel; }

		private:
			R3D_RiggedModel* m_handle;
			KinematicsModel* m_kmodel;

	};

	class ModelSystem {

		public:
			ModelSystem();
			~ModelSystem();

			RG_DECLSPEC void UpdateComponents();

			RG_DECLSPEC ModelComponent* NewModelComponent(R3D_StaticModel* model);
			RG_DECLSPEC void DeleteModelComponent(ModelComponent* comp);

			RG_DECLSPEC RiggedModelComponent* NewRiggedModelComponent(R3D_RiggedModel* model, KinematicsModel* kmodel);
			RG_DECLSPEC void DeleteRiggedModelComponent(RiggedModelComponent* comp);

			RG_INLINE ModelComponent* GetModelComponent(Uint32 idx) { return m_modelComponents[idx]; }
			RG_INLINE RiggedModelComponent* GetRiggedModelComponent(Uint32 idx) { return m_rmodelComponents[idx]; }

			RG_INLINE Uint32 GetModelCount() { return m_modelComponents.size(); }
			RG_INLINE Uint32 GetRiggedModelCount() { return m_rmodelComponents.size(); }

		private:

			Engine::PoolAllocator* m_alloc;
			Engine::PoolAllocator* m_ralloc;

			std::vector<ModelComponent*>	   m_modelComponents;
			std::vector<RiggedModelComponent*> m_rmodelComponents;

	};

}

#endif