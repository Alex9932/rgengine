#ifndef _ENTITYLIST_H
#define _ENTITYLIST_H

#include <uuid.h>
#include "component.h"
#include "popup.h"

class Viewport;

class EntityList : public UIComponent {
	public:
		EntityList(Viewport* vp);
		~EntityList();

		virtual void Draw();

	private:
		PopupID   m_popupidx;
		Viewport* m_vp;
};

#endif