/*

 Date: 10.12.23
*/

#ifndef _OBJIMPORTER_H
#define _OBJIMPORTER_H

#include "render.h"

namespace Engine {
	namespace Render {

		class ObjImporter : public ModelImporter {
			public:
				ObjImporter()  {}
				~ObjImporter() {}
				RG_DECLSPEC void ImportModel(String path, R3DCreateStaticModelInfo* info);
				RG_DECLSPEC void FreeModelData(R3DCreateStaticModelInfo* info);
		};

	}
}

#endif