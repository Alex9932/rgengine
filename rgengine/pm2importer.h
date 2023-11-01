/*

 Date: 12.10.23
*/

#ifndef _PM2IMPORTER_H
#define _PM2IMPORTER_H

#include "render.h"

namespace Engine {
	namespace Render {

		class PM2Importer : public ModelImporter {
			public:
				PM2Importer()  {}
				~PM2Importer() {}
				RG_DECLSPEC void ImportModel(String path, R3DCreateStaticModelInfo* info);
				RG_DECLSPEC void FreeModelData(R3DCreateStaticModelInfo* info);
		};

	}
}

#endif