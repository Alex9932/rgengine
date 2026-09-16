#ifndef _EXPORTER_H
#define _EXPORTER_H

#include "rgtypes.h"
#include "rendertypes.h"
#include "kinematicsmodel.h"

namespace Engine {
	class ModelExporter {
		public:
			ModelExporter() {}
			~ModelExporter() {}

			void ExportModel(String path, R3DStaticModelInfo* info, mat4* model = NULL) {}
	};

	class RiggedModelExporter {
		public:
			RiggedModelExporter() {}
			~RiggedModelExporter() {}

			void ExportRiggedModel(String path, R3DRiggedModelInfo* info, KinematicsModel* kmdl, mat4* model = NULL) {}
	};
}

#endif