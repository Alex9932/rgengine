#include "rgphysics.h"
#include "engine.h"

#include "rgmath.h"

#include "phcomponent.h"

#include "bullet/btBulletDynamicsCommon.h"

#define RG_PH_MOTIONSTATES 1024

namespace Engine {

	struct PhysicsWorld {
		btDiscreteDynamicsWorld*         world;
		btDefaultCollisionConfiguration* collision_config;
		btCollisionDispatcher*           collision_disp;
		btBroadphaseInterface*           broadphase_interface;
		btSequentialImpulseConstraintSolver* solver;

		btRigidBody* ground;
	};

	static btRigidBody* CreateGoundPlane(Allocator* alloc) {
		// Ground plane
		btScalar mass = 0;
		btBoxShape* shape = new btBoxShape(btVector3(btScalar(10.), btScalar(0.1), btScalar(10.)));
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(0, -0.1, 0));
		btDefaultMotionState motionState(transform);

		//btRigidBody::btRigidBodyConstructionInfo info(mass, &motionState, shape);
		btRigidBody::btRigidBodyConstructionInfo info(mass, 0, shape);
		btRigidBody* ground = RG_NEW_CLASS(alloc, btRigidBody)(info);

		ground->forceActivationState(DISABLE_DEACTIVATION);
		ground->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT);

		//ground->setMotionState(NULL);

		return ground;
	}

	static void DeleteGroundPlane(Allocator* alloc, btRigidBody* body) {
		delete body->getCollisionShape();
		RG_DELETE_CLASS(alloc, btRigidBody, body);
	}

	RGPhysics::RGPhysics() {
		m_isDisabled = false;
		m_alloc      = RG_NEW(STDAllocator)("Physics alloc");
		m_mstates    = RG_NEW_CLASS(m_alloc, PoolAllocator)("PH Motion shape pool", RG_PH_MOTIONSTATES, sizeof(btMotionState));

		m_world = (PhysicsWorld*)m_alloc->Allocate(sizeof(PhysicsWorld));

		m_world->collision_config     = new btDefaultCollisionConfiguration();
		m_world->collision_disp       = new btCollisionDispatcher(m_world->collision_config);
		m_world->broadphase_interface = new btDbvtBroadphase();
		m_world->solver               = new btSequentialImpulseConstraintSolver;

		m_world->world = new btDiscreteDynamicsWorld(m_world->collision_disp, m_world->broadphase_interface, m_world->solver, m_world->collision_config);

		m_world->world->setGravity(btVector3(0, -9.81, 0));

		m_world->ground = CreateGoundPlane(m_alloc);
		m_world->world->addRigidBody(m_world->ground);
	}

	RGPhysics::~RGPhysics() {
		m_world->world->removeRigidBody(m_world->ground);
		DeleteGroundPlane(m_alloc, m_world->ground);

		delete m_world->world;
		delete m_world->solver;
		delete m_world->broadphase_interface;
		delete m_world->collision_disp;
		delete m_world->collision_config;
		m_alloc->Deallocate(m_world);

		//RG_DELETE_CLASS(m_alloc, PoolAllocator, m_mstates);
		RG_DELETE(STDAllocator, m_alloc);
	}

	static Float32 windForce = 20;
	static Float32 windRndF  = 5;
	static vec3    windVec   = {0.8f, 0, -0.2f};

	void RGPhysics::StepSimulation() {
		Float64 dt = GetDeltaTime();

		if (!m_isDisabled) {

			// Apply 'wind'
			Float32 wx = (windVec.x * windForce) + (rgRandFloat() * windRndF);
			Float32 wy = (windVec.y * windForce) + (rgRandFloat() * 0.3f * windRndF);
			Float32 wz = (windVec.z * windForce) + (rgRandFloat() * windRndF);

			btVector3 wForce(wx, wy, wz);

			btAlignedObjectArray<btRigidBody*> array = m_world->world->getNonStaticRigidBodies();
			for (Uint32 i = 0; i < array.size(); i++) {
				array[i]->applyCentralForce(wForce);
			}
			

			m_world->world->stepSimulation(dt, 5);
		}

		std::vector<PHComponent*>::iterator pit = this->m_components.begin();
		for (; pit != this->m_components.end(); pit++) {
			(*pit)->Update(dt);
		}
	}

	void RGPhysics::ClearWorld() {
		btCollisionObjectArray array = m_world->world->getCollisionObjectArray();
		//array[0]->
	}

	PHComponent* RGPhysics::NewComponent() {
		PHComponent* comp = RG_NEW_CLASS(this->m_alloc, PHComponent)(this);
		this->m_components.push_back(comp);
		return comp;
	}

	void RGPhysics::DeleteComponent(PHComponent* comp) {
		std::vector<PHComponent*>::iterator it = this->m_components.begin();
		for (; it != this->m_components.end(); it++) {
			if (*it == comp) {
				*it = std::move(m_components.back());
				m_components.pop_back();
				RG_DELETE_CLASS(this->m_alloc, PHComponent, comp);
				break;
			}
		}
	}

	btDefaultMotionState* RGPhysics::AllocateMotionState() {
		return RG_NEW_CLASS(m_mstates, btDefaultMotionState)();
	}

	void RGPhysics::DeleteMotionState(btDefaultMotionState* ptr) {
		RG_DELETE_CLASS(m_mstates, btDefaultMotionState, ptr);
	}

	btDiscreteDynamicsWorld* RGPhysics::GetWorld() {
		return m_world->world;
	}

}