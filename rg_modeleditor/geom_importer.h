#ifndef _GEOM_IMPORTER_H
#define _GEOM_IMPORTER_H

#include <rgtypes.h>
#include <rendertypes.h>

void FreeStaticModel(R3DStaticModelInfo* info);
void ImportStaticModel(String path, String file, R3DStaticModelInfo* info);

#endif