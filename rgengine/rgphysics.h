#ifndef _RGPHYSICS_H
#define _RGPHYSICS_H

#include "rgtypes.h"
#include "allocator.h"
#include <vector>

class btDefaultMotionState;
class btDiscreteDynamicsWorld;

namespace Engine {

	struct PhysicsWorld;
	class PHComponent;

	class RGPhysics {
		public:
			RGPhysics();
			~RGPhysics();

			void StepSimulation();
			void ClearWorld();

			RG_DECLSPEC PHComponent* NewComponent();
			RG_DECLSPEC void DeleteComponent(PHComponent* comp);

			RG_INLINE PHComponent* GetComponent(Uint32 idx) { return m_components[idx]; }
			RG_INLINE Uint32 GetComponentCount() { return (Uint32)m_components.size(); }

			RG_DECLSPEC btDefaultMotionState* AllocateMotionState();
			RG_DECLSPEC void  DeleteMotionState(btDefaultMotionState* ptr);

			btDiscreteDynamicsWorld* GetWorld();

		private:

			//Engine::PoolAllocator*    m_alloc;
			std::vector<PHComponent*> m_components;

			STDAllocator*  m_alloc;   // Objects allocator
			PoolAllocator* m_mstates; // Motion state pool
			PhysicsWorld*  m_world;
	};

}

#endif