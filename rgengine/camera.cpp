#define DLL_EXPORT

#include "camera.h"

#include "rgmath.h"

namespace Engine {

	Camera::Camera(World* w, Float32 near, Float32 far, Float32 fov, Float32 aspect) : Entity(w) {
		this->m_near   = near;
		this->m_far    = far;
		this->m_fov    = fov;
		this->m_aspect = aspect;
		this->ReaclculateProjection();
	}

	Camera::~Camera() {
	}

	void Camera::ReaclculateProjection() {
		mat4_frustum(&this->m_proj, this->m_fov, this->m_aspect, this->m_near, this->m_far);
	}

	void Camera::Update(Float64 dt) {
	

	}

}