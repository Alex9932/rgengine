/*

 Date: 10.12.23
*/

#ifndef _OBJIMPORTER_H
#define _OBJIMPORTER_H

#include "importer.h"

namespace Engine {

	class ObjImporter : public ModelImporter {
		public:
			ObjImporter()  {}
			~ObjImporter() {}
			RG_DECLSPEC void ImportModel(String path, R3DStaticModelInfo* info);
			RG_DECLSPEC void FreeModelData(R3DStaticModelInfo* info);
	};

}

#endif