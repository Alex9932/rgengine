#ifndef _PHCOMPONENT_H
#define _PHCOMPONENT_H

#include "entity.h"

namespace Engine {

	class PHComponent : public Component {
		private:

		public:
			RG_DECLSPEC PHComponent();
			RG_DECLSPEC ~PHComponent();
			virtual void Destroy();

	};

}

#endif