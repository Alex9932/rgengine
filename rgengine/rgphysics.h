#ifndef _RGPHYSICS_H
#define _RGPHYSICS_H

#include "rgtypes.h"
#include "allocator.h"

namespace Engine {

	struct PhysicsWorld;

	class RGPhysics {
		public:
			RGPhysics();
			~RGPhysics();

			void StepSimulation();
			void ClearWorld();

		private:
			STDAllocator*  m_alloc; // Objects allocator
			PoolAllocator* m_mstates; // Motion state pool
			PhysicsWorld*  m_world;
	};

}

#endif