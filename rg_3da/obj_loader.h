#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#define CUSTOM_OBJ_ENABLED 0

#include <rgtypes.h>
#include <render.h>

#if CUSTOM_OBJ_ENABLED

R3D_StaticModel* OBJ_ToModel(String p);

#endif

#endif