#ifndef _ENTITYLIST_H
#define _ENTITYLIST_H

#include <uuid.h>
#include "component.h"
#include "popup.h"

class EntityList : public UIComponent {
	public:
		EntityList();
		~EntityList();

		virtual void Draw();

		RG_INLINE UUID GetActiveEntity() { return activeID; }

	private:
		PopupID popupidx;
		UUID    activeID;
};

#endif