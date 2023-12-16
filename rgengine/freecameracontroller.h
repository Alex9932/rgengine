#ifndef _FREECAMERACONTROLLER
#define _FREECAMERACONTROLLER

#include "camera.h"

namespace Engine {
	class FreeCameraController {
		public:
			RG_DECLSPEC FreeCameraController(Camera* camera);
			RG_DECLSPEC ~FreeCameraController();

			RG_DECLSPEC void Update();

		private:
			Camera* m_camptr = NULL;
			vec3    m_dir    = {};
			Float32 m_speed  = 0;

	};
}

#endif