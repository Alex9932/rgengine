#ifndef _GEOM_IMPORTER_H
#define _GEOM_IMPORTER_H

#include <rgtypes.h>
#include <rendertypes.h>
#include <importer.h>
#include <kinematicsmodel.h>
#include <map>

#include <assimp/scene.h>

struct ImportSceneData;

const aiScene* LoadScene(String path, String file, char* errorstr, size_t msglen, ImportSceneData** data, Bool animOnly = false);
void FreeScene(ImportSceneData* ptr);

class GeomImporter {
	private:
		char m_errorstr[1024];

	public:
		GeomImporter() { SDL_memset(m_errorstr, 0, 1024); }
		~GeomImporter() {}

		void ImportRiggedModel(ImportModelInfo* info);
		void FreeRiggedModelData(FreeModelInfo* data);

		Engine::KinematicsModel* LoadSkeleton(ImportModelInfo* info);

		const aiScene* GetAIScene(ImportModelInfo* info);

		String GetErrorString() { return m_errorstr; }
};

#endif