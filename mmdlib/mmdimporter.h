#ifndef _MMDIMPORTER_H
#define _MMDIMPORTER_H

#include <importer.h>
//#include <animation.h>

namespace Engine {
	class KinematicsModel;
	class Animation;
}

class PMDImporter : public Engine::ModelImporter, Engine::RiggedModelImporter {
	public:
		RG_INLINE PMDImporter()  {}
		RG_INLINE ~PMDImporter() {}

		RG_DECLSPEC void ImportModel(String path, R3DStaticModelInfo* info);
		RG_DECLSPEC void FreeModelData(R3DStaticModelInfo* info);

		RG_DECLSPEC void ImportRiggedModel(String path, R3DRiggedModelInfo* info);
		RG_DECLSPEC void FreeRiggedModelData(R3DRiggedModelInfo* info);

		RG_DECLSPEC Engine::KinematicsModel* ImportKinematicsModel(String path);

};

class PMXImporter : public Engine::ModelImporter, Engine::RiggedModelImporter {
	public:
		RG_INLINE PMXImporter() {}
		RG_INLINE ~PMXImporter() {}

		RG_DECLSPEC void ImportModel(String path, R3DStaticModelInfo* info);
		RG_DECLSPEC void FreeModelData(R3DStaticModelInfo* info);

		RG_DECLSPEC void ImportRiggedModel(String path, R3DRiggedModelInfo* info);
		RG_DECLSPEC void FreeRiggedModelData(R3DRiggedModelInfo* info);

		RG_DECLSPEC Engine::KinematicsModel* ImportKinematicsModel(String path);
};

class VMDImporter {
	public:
		RG_INLINE VMDImporter()  {}
		RG_INLINE ~VMDImporter() {}

		// TODO: Import animation from vmd file
		RG_DECLSPEC Engine::Animation* ImportAnimation(String path, Engine::KinematicsModel* model);
};

#endif