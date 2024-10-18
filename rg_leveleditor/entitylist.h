#ifndef _ENTITYLIST_H
#define _ENTITYLIST_H

#include "component.h"
#include "popup.h"

class EntityList : public UIComponent {
	public:
		EntityList();
		~EntityList();

		virtual void Draw();

	private:
		PopupID popupidx;
};

#endif