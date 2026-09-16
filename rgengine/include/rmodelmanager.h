#ifndef _RMODELMANAGER_H
#define _RMODELMANAGER_H

#include "rgtypes.h"
#include "rendertypes.h"
#include "render.h"
#include "kinematicsmodel.h"

namespace Engine {
	
	RG_DECLSPEC R3D_StaticModel* GetStaticModel(String name);
	RG_DECLSPEC void FreeStaticModel(R3D_StaticModel* mdl);

	RG_DECLSPEC R3D_RiggedModel* GetRiggedModel(String name);
	RG_DECLSPEC void FreeRiggedModel(R3D_RiggedModel* rmdl);

	RG_DECLSPEC KinematicsModel* GetUniqueKinematicsModel(R3D_RiggedModel* rmdl);
	RG_DECLSPEC void FreeUniqueKinematicsModel(KinematicsModel* km);

}

#endif