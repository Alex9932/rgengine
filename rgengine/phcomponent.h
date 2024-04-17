#ifndef _PHCOMPONENT_H
#define _PHCOMPONENT_H

#include "entity.h"

namespace Engine {

	struct PHBody;
	class RGPhysics;

	class PHComponent : public Component {
		private:
			PHBody* m_body;
			RGPhysics* m_ph;

		public:
			RG_DECLSPEC PHComponent(RGPhysics* ph);
			RG_DECLSPEC ~PHComponent();
			virtual void Destroy();

			void Update(Float64 dt);

	};

}

#endif