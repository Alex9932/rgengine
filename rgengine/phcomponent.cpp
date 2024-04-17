#include "phcomponent.h"
#include "engine.h"

#include "rgphysics.h"

#include "bullet/btBulletDynamicsCommon.h"

namespace Engine {

	struct PHBody {
		btRigidBody* body;

	};

	PHComponent::PHComponent(RGPhysics* ph) : Component(Component_PH) {

		m_ph = ph;
		m_body = RG_NEW(PHBody);

		btScalar mass = 10;
		btBoxShape* shape = new btBoxShape(btVector3(btScalar(0.5), btScalar(0.5), btScalar(0.5)));

		btVector3 localInertia(0, 0, 0);
		shape->calculateLocalInertia(mass, localInertia);

		btTransform transform;
		transform.setIdentity();

		vec3 pos = {0, 20, 0};// m_ent->GetTransform()->GetPosition();

		transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
		quat q = quat_axisAngle({ 0.633724, 0.443607, 0.633724, 1.0f});
		btQuaternion quater;
		quater.setX(q.x);
		quater.setY(q.y);
		quater.setZ(q.z);
		quater.setW(q.w);
		transform.setRotation(quater);

		//btDefaultMotionState* motionState = new btDefaultMotionState(transform);
		btDefaultMotionState* motionState = ph->AllocateMotionState();
		motionState->setWorldTransform(transform);

		btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, localInertia);
		m_body->body = RG_NEW(btRigidBody)(info);

		//m_body->body->applyTorqueImpulse(btVector3(0, 0, 10));

		ph->GetWorld()->addRigidBody(m_body->body);
	}
	
	PHComponent::~PHComponent() {
		
		m_ph->GetWorld()->removeRigidBody(m_body->body);
		m_ph->DeleteMotionState((btDefaultMotionState*)m_body->body->getMotionState());

		RG_DELETE(btRigidBody, m_body->body);
		RG_DELETE(PHBody, m_body);
	}

	void PHComponent::Destroy() {
		Engine::GetPhysics();
	}

	void PHComponent::Update(Float64 dt) {

		btTransform btransform = m_body->body->getWorldTransform();

#if 1
		mat4 m;
		btransform.getOpenGLMatrix((btScalar*)m.m);

		Transform* transform = m_ent->GetTransform();
		transform->Disable();

		transform->SetMatrix(&m);
#else
		vec3 pos;
		vec3 rot;

		pos.x = btransform.getOrigin().x();
		pos.y = btransform.getOrigin().y();
		pos.z = btransform.getOrigin().z();

		mat4 m;
		btransform.getOpenGLMatrix((btScalar*)m.m);

		if (m.m10 > 0.998) { // singularity at north pole
			rot.x = SDL_atan2f(m.m02, m.m22);
			rot.y = RG_PI / 2;
			rot.z = 0;
			return;
		}
		if (m.m10 < -0.998) { // singularity at south pole
			rot.x = SDL_atan2f(m.m02, m.m22);
			rot.y = -RG_PI / 2;
			rot.z = 0;
			return;
		}
		rot.x = SDL_atan2f(-m.m20, m.m00);
		rot.y = SDL_atan2f(-m.m12, m.m11);
		rot.z = SDL_asinf(m.m10);
		

		Transform* transform = m_ent->GetTransform();
		transform->SetPosition(pos);
		transform->SetRotation(rot);
#endif

	}

}