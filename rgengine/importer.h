#ifndef _IMPORTER_H
#define _IMPORTER_H

#include "rgtypes.h"
#include "rendertypes.h"

typedef struct NameField {
	char name[128];
	// For future use
} NameField;

typedef struct ModelExtraData {
	NameField* mesh_names;
	NameField* mat_names;
	NameField* bone_names;
} ModelExtraData;

typedef struct ImportModelInfo {
	String path;
	String file;
	union {
		R3DStaticModelInfo* as_static;
		R3DRiggedModelInfo* as_rigged;
	} info;
	ModelExtraData* extra;
	Bool skipFirstMat; // Need for skipping unused default material in some models
	const void* userdata;
} ImportModelInfo;

typedef struct FreeModelInfo {
	union {
		R3DStaticModelInfo* as_static;
		R3DRiggedModelInfo* as_rigged;
	} info;
	ModelExtraData* extra;
	const void* userdata;
} FreeModelInfo;

namespace Engine {
	class ModelImporter {
		public:
			ModelImporter() {}
			~ModelImporter() {}

			void ImportModel(ImportModelInfo* info) {}
			void FreeModelData(FreeModelInfo* data) {}
	};

	class RiggedModelImporter {
		public:
			RiggedModelImporter() {}
			~RiggedModelImporter() {}

			void ImportRiggedModel(ImportModelInfo* info) {}
			void FreeRiggedModelData(FreeModelInfo* data) {}
	};
}

#endif