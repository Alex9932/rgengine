#ifndef _ANIMIMPORTER_H
#define _ANIMIMPORTER_H

#include "rgtypes.h"
#include "animation.h"

namespace Engine {
	class AnimImporter {
		public:
			AnimImporter() {}
			~AnimImporter() {}
			RG_DECLSPEC Animation* ImportAnimation(String path);
	};
}

#endif