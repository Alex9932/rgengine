#ifndef _PM2EXPORTER
#define _PM2EXPORTER

#include "exporter.h"

namespace Engine {
	class PM2Exporter : public ModelExporter {
		public:
			PM2Exporter() {}
			~PM2Exporter() {}

			RG_DECLSPEC void ExportModel(String path, R3DStaticModelInfo* info, mat4* model = NULL);
	};
}

#endif