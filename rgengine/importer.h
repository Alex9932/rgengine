#ifndef _IMPORTER_H
#define _IMPORTER_H

#include "rgtypes.h"
#include "rendertypes.h"

namespace Engine {
	class ModelImporter {
		public:
			ModelImporter() {}
			~ModelImporter() {}

			void ImportModel(String path, R3DStaticModelInfo* info) {}
			void FreeModelData(R3DStaticModelInfo* info) {}
	};

	class RiggedModelImporter {
		public:
			RiggedModelImporter() {}
			~RiggedModelImporter() {}

			void ImportRiggedModel(String path, R3DRiggedModelInfo* info);
			void FreeRiggedModelData(R3DRiggedModelInfo* info);
	};
}

#endif