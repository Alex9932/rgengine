#ifndef _RMODELMANAGER_H
#define _RMODELMANAGER_H

#include "rgtypes.h"
#include "rendertypes.h"
#include "render.h"

namespace Engine {
	
	R3D_StaticModel* GetStaticModel(String path);
	void FreeStaticModel(R3D_StaticModel* mdl);

	R3D_RiggedModel* GetRiggedModel(String path);
	void FreeRiggedModel(R3D_RiggedModel* rmdl);

}

#endif