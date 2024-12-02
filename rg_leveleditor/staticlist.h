#ifndef _STATICLIST_H
#define _STATICLIST_H

#include "component.h"

class Viewport;

class StaticList : public UIComponent {
	public:
		StaticList(Viewport* vp);
		~StaticList();

		virtual void Draw();

	private:
		Viewport* m_vp = NULL;

};

#endif