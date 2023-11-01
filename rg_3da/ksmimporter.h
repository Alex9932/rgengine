#ifndef _KSMIMPORTER_H
#define _KSMIMPORTER_H

#include "render.h"

class KSMImporter : public Engine::Render::ModelImporter {
	private:
		char assets[256];

	public:
		RG_INLINE KSMImporter(String assets_path) { SDL_snprintf(assets, 256, "%s", assets_path); }
		RG_INLINE ~KSMImporter() {}

		void ImportModel(String path, R3DCreateStaticModelInfo* info);
		void FreeModelData(R3DCreateStaticModelInfo* info);

};

#endif