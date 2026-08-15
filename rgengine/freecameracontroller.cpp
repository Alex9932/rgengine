#define DLL_EXPORT

#include "freecameracontroller.h"
#include "input.h"

#include "engine.h"

namespace Engine {

	FreeCameraController::FreeCameraController(Camera* camera) {
		m_camptr = camera;
	}

	FreeCameraController::~FreeCameraController() { }

	void FreeCameraController::Update() {

		Float64 dt = GetDeltaTime();
		Float32 sens = 0.1f;

		vec2 raw_delta = { 0 };

		// Angles

		if (IsButtonDown(3)) {
			raw_delta.x = (Float32)GetMouseDX();
			raw_delta.y = (Float32)GetMouseDY();
		}

		Float32 t = 1.0f - SDL_expf(-this->m_mouse_smooth_rate * (Float32)dt);
		this->m_smouse = this->m_smouse + (raw_delta - this->m_smouse) * t;

		this->m_angles.x -= this->m_smouse.y * sens;
		this->m_angles.y -= this->m_smouse.x * sens;
		this->m_angles.z = 0;

		// Movement

		vec3 cam_fwd = { 0, 0, -1 };
		vec3 cam_up  = { 0, 1, 0 };

		vec3 fwd = {};
		vec3 up  = {};

		vec3_rotate(&fwd, cam_fwd, this->m_angles);
		vec3_rotate(&up,  cam_up,  this->m_angles);

		vec3 left = up.cross(fwd);

		Float32 strafe_input = 0.0f;
		Float32 max_speed = 1.0f;
		vec3 wish_dir = { 0 };

		// Input
		if (IsKeyDown(SDL_SCANCODE_W)) { wish_dir += fwd;  }
		if (IsKeyDown(SDL_SCANCODE_S)) { wish_dir -= fwd;  }
		if (IsKeyDown(SDL_SCANCODE_A)) { wish_dir += left; strafe_input += 1.0f; }
		if (IsKeyDown(SDL_SCANCODE_D)) { wish_dir -= left; strafe_input -= 1.0f; }
		if (IsKeyDown(SDL_SCANCODE_SPACE))  { wish_dir += up; }
		if (IsKeyDown(SDL_SCANCODE_LSHIFT)) { wish_dir -= up; }

		if (IsKeyDown(SDL_SCANCODE_LCTRL)) { max_speed = 5;    }
		if (IsKeyDown(SDL_SCANCODE_LALT))  { max_speed = 0.1f; }

		vec3 wish_velocity = wish_dir.normalize_safe() * max_speed;

		Float32 accel_rate = (wish_dir.length() > 0.0f) ? this->m_accel : this->m_friction;
		t = 1.0f - SDL_expf(-accel_rate * (Float32)dt);
		this->m_dir = this->m_dir + (wish_velocity - this->m_dir) * t;

		// Roll

		Float32 rtarg = strafe_input * this->m_rmax;
		t = 1.0f - SDL_expf(-this->m_roll_speed * (Float32)dt);
		this->m_roll = this->m_roll + (rtarg - this->m_roll) * t;

		// Apply

		vec3 pos = this->m_camptr->GetTransform()->GetPosition();
		pos += this->m_dir * (Float32)dt;
		this->m_angles.z = this->m_roll;
		this->m_camptr->GetTransform()->SetPosition(pos);
		this->m_camptr->GetTransform()->SetRotation(this->m_angles);

	}
}