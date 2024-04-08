#include "phcomponent.h"
#include "engine.h"

namespace Engine {

	PHComponent::PHComponent() : Component(Component_PH) {

	}
	
	PHComponent::~PHComponent() {

	}

	void PHComponent::Destroy() {
		Engine::GetPhysics();
	}

	void PHComponent::Update(Float64 dt) {

	}

}