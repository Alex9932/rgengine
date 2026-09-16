#ifndef _FREECAMERACONTROLLER
#define _FREECAMERACONTROLLER

#include "camera.h"

namespace Engine {
	class FreeCameraController {
		public:
			RG_DECLSPEC FreeCameraController(Camera* camera);
			RG_DECLSPEC ~FreeCameraController();

			RG_DECLSPEC void Update();

			RG_INLINE void SetAngles(const vec3& angles) { m_angles = angles; }

		private:
			Camera* m_camptr = NULL;
			vec3    m_dir    = {};
			vec3    m_angles = {};
			vec2    m_smouse = {};
			Float32 m_roll   = 0;

		public:
			Float32 m_accel             = 20.0f;
			Float32 m_friction          = 18.0f;
			Float32 m_mouse_smooth_rate = 25.0f;
			Float32 m_roll_speed        = 8.0f;
			Float32 m_rmax              = 0.025f;

	};
}

#endif