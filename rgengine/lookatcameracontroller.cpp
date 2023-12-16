#define DLL_EXPORT
#include "lookatcameracontroller.h"
#include "input.h"

namespace Engine {


	LookatCameraController::LookatCameraController(Camera* camera) {
		m_camptr = camera;
		m_center = { 0, 1, 0 };
		m_length = 1;
	}

	LookatCameraController::~LookatCameraController() { }

	void LookatCameraController::Update() {

		Transform* camTransform = m_camptr->GetTransform();

		Float32 sens = 0.1f;

		if (IsButtonDown(3)) {
			Float64 dx = GetMouseDX();
			Float64 dy = GetMouseDY();

			m_angles.x -= dx * sens;
			m_angles.y += dy * sens;

			vec3 rot = { m_angles.y, -m_angles.x, 0};
			camTransform->SetRotation(rot);

		}

		Float32 mwheel = (Float32)GetMouseDW() * 0.123f;
		m_length -= mwheel;

		// Recalculate position;

		vec3 camera_coord;

		camera_coord.x = SDL_sinf(m_angles.x) * SDL_cosf(m_angles.y) * m_length;
		camera_coord.y = SDL_sinf(m_angles.y) * m_length;
		camera_coord.z = SDL_cosf(m_angles.x) * SDL_cosf(m_angles.y) * m_length;

		camTransform->SetPosition(m_center + camera_coord);
		//camTransform->SetPosition(camera_coord);

		//rgLogInfo(RG_LOG_SYSTEM, "Camera at: %f %f %f, len: %f, Angles: %f %f", camera_coord.x, camera_coord.y, camera_coord.z, m_length, m_angles.x, m_angles.y);


		vec3    m_center;
		Float32 m_length;

	}

}