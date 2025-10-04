#ifndef _GEOM_IMPORTER_H
#define _GEOM_IMPORTER_H

#include <rgtypes.h>
#include <rendertypes.h>
#include <importer.h>

#include <assimp/scene.h>

const aiScene* LoadScene(String path, String file);

class GeomImporter {
	public:
		GeomImporter() {}
		~GeomImporter() {}

		void ImportRiggedModel(ImportModelInfo* info);
		void FreeRiggedModelData(FreeModelInfo* data);
};

#endif