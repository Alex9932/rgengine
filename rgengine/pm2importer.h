/*
 * rgEngine importer/pm2importer.h
 *
 *  Created on: Oct 12, 2023
 *      Author: alex9932
*/

#ifndef _PM2IMPORTER_H
#define _PM2IMPORTER_H

#include "importer.h"
#include "kinematicsmodel.h"

namespace Engine {

	class PM2Importer : public ModelImporter, RiggedModelImporter {
		public:
			PM2Importer()  {}
			~PM2Importer() {}
			//RG_DECLSPEC void ImportModel(String path, R3DStaticModelInfo* info);
			//RG_DECLSPEC void FreeModelData(R3DStaticModelInfo* info);
			
			RG_DECLSPEC void ImportModel(ImportModelInfo* info);
			RG_DECLSPEC void FreeModelData(FreeModelInfo* data);

			RG_DECLSPEC void ImportRiggedModel(ImportModelInfo* info);
			RG_DECLSPEC void FreeRiggedModelData(FreeModelInfo* data);

			RG_DECLSPEC KinematicsModel* LoadSkeleton(ImportModelInfo* info);
	};
}

#endif