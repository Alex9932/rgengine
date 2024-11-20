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
			Float32 m_speed  = 0;
			vec3    m_angles = {};

	};
}

#endif