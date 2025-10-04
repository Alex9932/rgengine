/*

 Date: 10.12.23
*/

#if 0 // DISABLE OBJIMPORTER

#ifndef _OBJIMPORTER_H
#define _OBJIMPORTER_H

#include "importer.h"

namespace Engine {

	class ObjImporter : public ModelImporter {
		public:
			ObjImporter()  {}
			~ObjImporter() {}
			RG_DECLSPEC void ImportModel(ImportModelInfo* info);
			RG_DECLSPEC void FreeModelData(R3DStaticModelInfo* info, ModelExtraData* data);
	};

}

#endif

#endif