#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include "component.h"
#include <camera.h>

class Viewport : public UIComponent {
	public:
		Viewport(Engine::Camera* camera);
		~Viewport();

		virtual void Draw();
		virtual void OnResize(ivec2 newsize);

		void SetImGuizmoRect();

	private:
		Engine::Camera* m_camera;

};

#endif