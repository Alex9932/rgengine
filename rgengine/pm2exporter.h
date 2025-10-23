#ifndef _PM2EXPORTER
#define _PM2EXPORTER

#include "exporter.h"

namespace Engine {
	class PM2Exporter : public ModelExporter, RiggedModelExporter {
		public:
			PM2Exporter() {}
			~PM2Exporter() {}

			RG_DECLSPEC void ExportModel(String path, R3DStaticModelInfo* info, mat4* model = NULL);
			RG_DECLSPEC void ExportModel(String path, R3DRiggedModelInfo* info, mat4* model = NULL);  // Export rigged model as static
			RG_DECLSPEC void ExportRiggedModel(String path, R3DRiggedModelInfo* info, KinematicsModel* kmdl, mat4* model = NULL);
	};
}

#endif