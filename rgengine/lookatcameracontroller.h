#ifndef _LOOKATCAMERACONTROLLER
#define _LOOKATCAMERACONTROLLER

#include "camera.h"
#include "rgmath.h"

namespace Engine {
	class LookatCameraController {
		public:
			RG_DECLSPEC LookatCameraController(Camera* camera);
			RG_DECLSPEC ~LookatCameraController();

			RG_DECLSPEC void Update();

		private:
			Camera* m_camptr = NULL;
			vec3    m_center = {};
			vec2    m_angles = {};
			Float32 m_length = 0;

	};
}

#endif