#ifndef _PHCOMPONENT_H
#define _PHCOMPONENT_H

#include "entity.h"

namespace Engine {

	struct PHBody;

	class PHComponent : public Component {
		private:
			PHBody* m_body;

		public:
			RG_DECLSPEC PHComponent();
			RG_DECLSPEC ~PHComponent();
			virtual void Destroy();

			void Update(Float64 dt);

	};

}

#endif