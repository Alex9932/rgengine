#ifndef _LEVELLOADER_H
#define _LEVELLOADER_H

#include "rgtypes.h"
#include "rgvector.h"

typedef struct LevelHeader {
	Uint32 version;
	Uint32 compver; // Compiler version
	Uint32 models;
	Uint32 entities;
	char   compiler[64];
} LevelHeader;

typedef struct LevelEntity {
	Uint32 mdlid;
	vec3   pos;
	vec3   rot;
	vec3   scale;
} LevelEntity;

typedef struct LevelGeomHeader {
	Uint32 version;
	Uint32 compver;
	Uint32 objects;
} LevelGeomHeader;

namespace Engine {

}

#endif