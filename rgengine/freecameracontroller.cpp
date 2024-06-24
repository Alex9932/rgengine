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

		vec3 rot = m_camptr->GetTransform()->GetRotation();

		Float32 sens = 0.1f;

		if (IsButtonDown(3)) {
			Float64 dx = GetMouseDX();
			Float64 dy = GetMouseDY();

			rot.x += dy * sens;
			rot.y += dx * sens;

			m_camptr->GetTransform()->SetRotation(rot);
#if 0
			rgLogInfo(RG_LOG_SYSTEM, "Pressed %f %f", rot.x, rot.y);
#endif
		}

		vec3 cam_fwd = { 0, 0, -1 };
		vec3 cam_up  = { 0, 1, 0 };
		vec3 rotated_fwd = {};
		vec3 rotated_up = {};

		vec3_rotate(&rotated_fwd, cam_fwd, rot);
		vec3_rotate(&rotated_up,  cam_up,  rot);
#if 0
		mat4 view_matrix = MAT4_IDENTITY();
		//mat4_rotatey(&view_matrix, -rot.y);
		//mat4_rotate(&view_matrix, { -rot.x, -rot.y, 0});

		mat4 rx, ry, rz, ryz;
		mat4_rotatex(&rx, -rot.x);
		mat4_rotatey(&ry, -rot.y);
		mat4_rotatez(&rz, -rot.z);
		ryz = rz * ry;
		view_matrix = ryz * rx;


		vec3 rotated_fwd = view_matrix * cam_fwd;
		vec3 rotated_up = view_matrix * cam_up;
#endif

#if 0
		vec3 rotated_fwd;
		rotated_fwd.x = SDL_cosf(rot.x) * SDL_cosf(rot.y);
		rotated_fwd.y = SDL_sinf(rot.y);
		rotated_fwd.z = SDL_sinf(rot.x) * SDL_cosf(rot.y);
		rotated_fwd = rotated_fwd.normalize();
#endif

		vec3 left = rotated_up.cross(rotated_fwd);
#if 0
		rgLogInfo(RG_LOG_SYSTEM, "Fwd: %f %f %f, Rotated: %f %f %f",
			cam_fwd.x, cam_fwd.y, cam_fwd.z,
			rotated_fwd.x, rotated_fwd.y, rotated_fwd.z);
#endif	
		this->m_speed = 1;

		if (IsKeyDown(SDL_SCANCODE_W)) {
			this->m_dir += rotated_fwd;
		}
		if (IsKeyDown(SDL_SCANCODE_S)) {
			this->m_dir -= rotated_fwd;
		}

		if (IsKeyDown(SDL_SCANCODE_A)) {
			this->m_dir += left;
		}
		if (IsKeyDown(SDL_SCANCODE_D)) {
			this->m_dir -= left;
		}

		if (IsKeyDown(SDL_SCANCODE_SPACE)) {
			this->m_dir += rotated_up;
		}
		if (IsKeyDown(SDL_SCANCODE_LSHIFT)) {
			this->m_dir -= rotated_up;
		}

		if (IsKeyDown(SDL_SCANCODE_LCTRL)) {
			this->m_speed = 5;
		}

		if (IsKeyDown(SDL_SCANCODE_LALT)) {
			this->m_speed = 0.1f;
		}

		this->m_dir = this->m_dir.normalize_safe();

		vec3 pos = m_camptr->GetTransform()->GetPosition();
		pos += (this->m_dir * this->m_speed) * dt;
		m_camptr->GetTransform()->SetPosition(pos);

		//this->m_dir -= this->m_dir * 0.6f * dt;
#if 0
		rgLogInfo(RG_LOG_SYSTEM, "Angles: %f %f -> Fwd: %f %f %f",
			rot.x, rot.y, this->m_dir.x, this->m_dir.y, this->m_dir.z);
#endif

		this->m_dir = this->m_dir * 0;



#if 0
		vec3 d = { 0, 0, 0 };

		Float32 maxSpeed = 1.0f;
		Float32 moveSpeed = 0.1f;
		Float32 acceleration = 0.1f;

		Bool isKeyDown = false;

		if (IsKeyDown(SDL_SCANCODE_S)) {
			d.z += moveSpeed;
			isKeyDown = true;
		}
		if (IsKeyDown(SDL_SCANCODE_W)) {
			d.z -= moveSpeed;
			isKeyDown = true;
		}
		if (IsKeyDown(SDL_SCANCODE_D)) {
			d.x += moveSpeed;
			isKeyDown = true;
		}
		if (IsKeyDown(SDL_SCANCODE_A)) {
			d.x -= moveSpeed;
			isKeyDown = true;
		}

		this->m_dir += d;
		this->m_dir = this->m_dir.normalize_safe();

		if (isKeyDown) {
			this->m_speed += acceleration;
		}

		if (this->m_speed > maxSpeed) { this->m_speed = maxSpeed; }
		if (this->m_speed < 0) { this->m_speed = 0; }

		this->m_speed -= 0.6f * dt;

		vec3 pos = this->GetTransform()->GetPosition();
		pos += (this->m_dir * this->m_speed) * dt;
		this->GetTransform()->SetPosition(pos);
#endif

	}

}