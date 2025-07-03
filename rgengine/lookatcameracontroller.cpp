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

	void LookatCameraController::SetLookAtPosition(vec3* pos) {
		m_center = *pos;
	}

	void LookatCameraController::Update() {

		Transform* camTransform = m_camptr->GetTransform();

		Float32 sens  = 0.025f;
		Float32 msens = 1.0f;   // Move speed
		Float32 wsens = 1.0f;   // Wheel speed

		if (IsKeyDown(SDL_SCANCODE_LSHIFT)) {
			msens = 0.1f;
			wsens = 0.5f;
		}

		if (IsKeyDown(SDL_SCANCODE_LCTRL)) {
			msens = 10.0f;
			wsens = 15.0f;
		}

		if (IsButtonDown(3)) {
			Float64 dx = GetMouseDX();
			Float64 dy = GetMouseDY();

			m_angles.x -= dx * sens * 2;
			m_angles.y += dy * sens * 2;

			vec3 rot = { m_angles.y, -m_angles.x, 0};
			camTransform->SetRotation(rot);

		}

		if (IsButtonDown(2)) {
			vec3 cam_fwd = { 0, 0, -1 };
			vec3 cam_up = { 0, 1, 0 };

			mat4 view_matrix = MAT4_IDENTITY();

			vec3 rot = m_camptr->GetTransform()->GetRotation();
			mat4 rx, ry, rz, ryz;
			mat4_rotatex(&rx, -rot.x);
			mat4_rotatey(&ry, -rot.y);
			mat4_rotatez(&rz, -rot.z);
			ryz = rz * ry;
			view_matrix = ryz * rx;

			vec3 rotated_fwd = view_matrix * cam_fwd;
			vec3 rotated_up = view_matrix * cam_up;
			vec3 left = rotated_up.cross(rotated_fwd);

			vec3 delta = {};
			delta += left       * GetMouseDX() * sens * msens;
			delta += rotated_up * GetMouseDY() * sens * msens;

			m_center += delta;
		}

		Float32 mwheel = (Float32)GetMouseDW() * sens * wsens;
		m_length -= mwheel;
		if (m_length < 0) { m_length = 0; }

		// Recalculate position;

		vec3 camera_coord;

		camera_coord.x = SDL_sinf(m_angles.x) * SDL_cosf(m_angles.y) * m_length;
		camera_coord.y = SDL_sinf(m_angles.y) * m_length;
		camera_coord.z = SDL_cosf(m_angles.x) * SDL_cosf(m_angles.y) * m_length;

		camTransform->SetPosition(m_center + camera_coord);
		//camTransform->SetPosition(camera_coord);

		//rgLogInfo(RG_LOG_SYSTEM, "Camera at: %f %f %f, len: %f, Angles: %f %f", camera_coord.x, camera_coord.y, camera_coord.z, m_length, m_angles.x, m_angles.y);


	}

}