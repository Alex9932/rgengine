#ifndef _ANIMEXPORTER
#define _ANIMEXPORTER

#include "rgtypes.h"
#include "animation.h"

namespace Engine {
	class AnimExporter {
		public:
			AnimExporter() {}
			~AnimExporter() {}
			RG_DECLSPEC void ExportAnimation(String path, Animation* anim);
	};
}

#endif