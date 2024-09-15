#ifndef _LEVELLOADER_H
#define _LEVELLOADER_H

#include "rgtypes.h"
#include "rgvector.h"

/*

level format:

gamedata/levels/%levelname%/:
	geometry/:        // in level models for static geometry (to use this must be set flag in level description)
		model0.pm2
		model1.pm2
		model2.pm2
		model3.pm2
		&textures&
	light/:           // in future
		&lightmaps&.png
	scripts/:         // level scripts
		&script&.js
	textures/:        // textures used by level.geom
		&textures&.png
	level             // level description
	level.geom        // level static geometry (ground, roads, buildings)
	level.light       // light description
	spawn             // level entities


[RGLevelDescriptor]
	uint32  sig;       // Signature 'rg\0\0'
	uint16  version;   // Format version
	uint16  compver;   // Compiler version

---------------------------------------------------

Level description
Non-static objects
...

 => level binary file:

[zstring]
	uint32 length;
	char[] data;

[header]
	RGLevelDescriptor desc;
	char#64 compiler;  // 64-byte string (fixed size zstring)
	uint32  models;    // Count of used models

[model]
	zstring name;     // "modelname" - use global model (in gamedata/models) | "&modelname" - use level's model (in %levelname%/geometry)
	vec3    pos;      // position in world
	vec3    rot;      // rotation


File structure:
	[header]
	[model]
	[model]
	...


---------------------------------------------------

Level static geometry

 => level.geom binary file:

[header]
	RGLevelDescriptor desc;
	uint32 textures;
	uint32 vertexbuffers;

[texture]
	zstring texture_name; // "texturename" - use global texture (in gamedata/textures) | "&texturename" - use level's texture (in %levelname%/textures)

[vertexbuffer]
	uint32 flags;       // prop | terrain | other
	uint32 vertex_count;
	uint32 geom_count;
	vertex vertices[];
	geom   geometries[];

[vertex]
	vec3 pos;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
	vec2 lightmap; // future use (for static light)

[geom]
	uint32 textureid;
	uint32 index_count;
	vec3   aabb_min;
	vec3   aabb_max;
	uint32 indices[];


File structure:
[header]
[texture]
...
[vertexbuffer]
...

---------------------------------------------------

Light source information

=> level.light binary file:

[header]
	RGLevelDescriptor desc;
	uint32 point_sources;   // Point light sources
	uint32 spot_sources;    // Spot light sources

[point_source]
	vec3    pos;
	Float32 intensity;
	vec3    color;
	Float32 radius;    // For rendering optimizations
	Uint32  flags;     // is static | cast shadow

[spot_source]
	vec3    pos;
	Float32 intensity;
	vec3    color;
	Float32 length;    // For rendering optimizations
	vec3    orientation;
	Uint32  flags;     // is static | cast shadow
	...

File structure:
[header]
[point_source]
...
[spot_source]
...

---------------------------------------------------

// NPC (future use)

 => spawn binary file:

[header]
	RGLevelDescriptor desc;
	uint32 entities;   // Count of entityes

[entity]
	uint32  modelid;
	vec3    position;
	vec3    rotation;
	vec3    scale;

File structure:
[header]
[entity]
[entity]
...

*/

typedef struct RGLevelDescriptor {
	Uint32 sig;     // Signature 'rg\0\0'
	Uint16 version; // Level version
	Uint16 compver; // Compiler version
} RGLevelDescriptor;

typedef struct LevelHeader {
	RGLevelDescriptor desc;
	char   compiler[64];
	Uint32 models;
} LevelHeader;

typedef struct LevelGeomHeader {
	RGLevelDescriptor desc;
	Uint32 objects;
} LevelGeomHeader;

typedef struct LevelLightHeader {
	RGLevelDescriptor desc;
	Uint32 point_sources;
	Uint32 spot_sources;
} LevelLightHeader;

typedef struct PointSource {
	vec3    pos;
	Float32 intensity;
	vec3    color;
	Float32 radius;
} PointSource;

typedef struct SpotSource {
	vec3    pos;
	Float32 intensity;
	vec3    color;
	Float32 length;
	vec3    orientation;
} SpotSource;


typedef struct SpawnHeader {
	RGLevelDescriptor desc;
	Uint32 entities;
} SpawnHeader;

// Temporary
typedef struct LevelEntity {
	Uint32 mdlid;
	vec3   pos;
	vec3   rot;
	vec3   scale;
} LevelEntity;

namespace Engine {

}

#endif