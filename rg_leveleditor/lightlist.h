#ifndef _LIGHTLIST_H
#define _LIGHTLIST_H

#include "component.h"

class Viewport;

class LightList : public UIComponent {
	public:
		LightList(Viewport* vp);
		~LightList();

		virtual void Draw();

	private:
		Viewport* m_vp = NULL;

};

#endif