#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include "component.h"

class Viewport : public UIComponent {
	public:
		Viewport();
		~Viewport();

		virtual void Draw();
		virtual void OnResize(ivec2 newsize);

	private:

};

#endif