#ifndef _ENTITYLIST_H
#define _ENTITYLIST_H

#include "component.h"

class EntityList : public UIComponent {
	public:
		EntityList();
		~EntityList();

		virtual void Draw();
};

#endif