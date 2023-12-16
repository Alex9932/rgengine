/*

 Date: 12.10.23
*/

#ifndef _PM2IMPORTER_H
#define _PM2IMPORTER_H

#include "importer.h"

namespace Engine {

	class PM2Importer : public ModelImporter {
		public:
			PM2Importer()  {}
			~PM2Importer() {}
			RG_DECLSPEC void ImportModel(String path, R3DStaticModelInfo* info);
			RG_DECLSPEC void FreeModelData(R3DStaticModelInfo* info);
	};
}

#endif