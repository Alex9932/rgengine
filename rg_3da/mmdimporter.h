#ifndef _MMDIMPORTER_H
#define _MMDIMPORTER_H

#include "render.h"
//#include <animation.h>

namespace Engine {
	class KinematicsModel;
	class Animation;
}

class PMDImporter : public Engine::Render::ModelImporter {
	public:
		RG_INLINE PMDImporter()  {}
		RG_INLINE ~PMDImporter() {}

		void ImportModel(String path, R3DCreateStaticModelInfo* info);
		void FreeModelData(R3DCreateStaticModelInfo* info);

		void ImportRiggedModel(String path, R3DCreateRiggedModelInfo* info);
		void FreeRiggedModelData(R3DCreateRiggedModelInfo* info);

		Engine::KinematicsModel* ImportKinematicsModel(String path);

};

class VMDImporter {
	public:
		RG_INLINE VMDImporter()  {}
		RG_INLINE ~VMDImporter() {}

		// TODO: Import animation from vmd file
		Engine::Animation* ImportAnimation(String path, Engine::KinematicsModel* model);
};

#endif