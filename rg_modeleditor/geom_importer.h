#ifndef _GEOM_IMPORTER_H
#define _GEOM_IMPORTER_H

#include <rgtypes.h>
#include <rendertypes.h>

typedef struct NameField {
	char name[128];
	// For future use
} NameField;

typedef struct ModelExtraData {
	NameField* mesh_names;
	NameField* mat_names;
} ModelExtraData;

typedef struct ImportStaticModelInfo {
	String path;
	String file;
	R3DStaticModelInfo* info;
	ModelExtraData* extra; // For saving extra model data (for example: material and mesh names)
	Bool skipFirstMat; // Need for skipping unused default material in some models
} ImportStaticModelInfo;

typedef struct ImportRiggedModelInfo {
	String path;
	String file;
	R3DRiggedModelInfo* info;
	ModelExtraData* extra;
	Bool skipFirstMat;

} ImportRiggedModelInfo;

void FreeStaticModel(R3DStaticModelInfo* info, ModelExtraData* extra);
void ImportStaticModel(ImportStaticModelInfo* info);

void FreeRiggedModel(R3DRiggedModelInfo* info, ModelExtraData* extra);
void ImportRiggedModel(ImportRiggedModelInfo* info);

#endif