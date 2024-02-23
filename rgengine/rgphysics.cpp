#include "rgphysics.h"
#include "engine.h"

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

	static btRigidBody* CreateGoundPlane(Allocator* alloc, PoolAllocator* mstates) {
		// Ground plane
		btScalar mass = 0;
		btBoxShape* shape = new btBoxShape(btVector3(btScalar(10.), btScalar(0.1), btScalar(10.)));
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(0, -0.1, 0));
		btDefaultMotionState* myMotionState = RG_NEW_CLASS(mstates, btDefaultMotionState)(transform);

		btRigidBody::btRigidBodyConstructionInfo info(mass, myMotionState, shape);
		btRigidBody* ground = RG_NEW_CLASS(alloc, btRigidBody)(info);

		ground->forceActivationState(DISABLE_DEACTIVATION);
		ground->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT);

		return ground;
	}

	static void DeleteGroundPlane(Allocator* alloc, PoolAllocator* mstates, btRigidBody* body) {
		delete body->getCollisionShape();
		btDefaultMotionState* mState = (btDefaultMotionState*)body->getMotionState();
		RG_DELETE_CLASS(mstates, btDefaultMotionState, mState);
		RG_DELETE_CLASS(alloc, btRigidBody, body);
	}

	RGPhysics::RGPhysics() {
		m_alloc = RG_NEW(STDAllocator)("Physics alloc");
		m_mstates = RG_NEW_CLASS(m_alloc, PoolAllocator)("PH Motion shape pool", RG_PH_MOTIONSTATES, sizeof(btMotionState));

		m_world = (PhysicsWorld*)m_alloc->Allocate(sizeof(PhysicsWorld));

		m_world->collision_config     = new btDefaultCollisionConfiguration();
		m_world->collision_disp       = new btCollisionDispatcher(m_world->collision_config);
		m_world->solver               = new btSequentialImpulseConstraintSolver;
		m_world->broadphase_interface = new btDbvtBroadphase();

		m_world->world = new btDiscreteDynamicsWorld(m_world->collision_disp, m_world->broadphase_interface, m_world->solver, m_world->collision_config);

		m_world->world->setGravity(btVector3(0, -9.81, 0));

		m_world->ground = CreateGoundPlane(m_alloc, m_mstates);
		m_world->world->addRigidBody(m_world->ground);
	}

	RGPhysics::~RGPhysics() {
		m_world->world->removeRigidBody(m_world->ground);
		DeleteGroundPlane(m_alloc, m_mstates, m_world->ground);

		delete m_world->world;
		delete m_world->solver;
		delete m_world->broadphase_interface;
		delete m_world->collision_disp;
		delete m_world->collision_config;
		m_alloc->Deallocate(m_world);

		RG_DELETE_CLASS(m_alloc, PoolAllocator, m_mstates);
		RG_DELETE(STDAllocator, m_alloc);
	}

	void RGPhysics::StepSimulation() {
		m_world->world->stepSimulation(GetDeltaTime(), 5);
	}

	void RGPhysics::ClearWorld() {
		btCollisionObjectArray array = m_world->world->getCollisionObjectArray();
		//array[0]->
	}

}