#include "levelloader.h"

/*

level format:

gamedata/levels/%levelname%/:
	geometry/:        // in level models (to use this must be set flag in level description)
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
		&lightmaps&.png
	level             // level description
	light             // light description
	level.geom        // level static geometry in pm2 + physics (ground, roads, buildings)

level binary file:

[zstring]
	uint32 length;
	char[] data;

[header]
	uint32  version;   // Format version
	uint32  compver;   // Compiler version
	uint32  models;    // Count of used models
	uint32  entities;  // Count of entityes
	char#64 compiler;  // 64-byte string (fixed size zstring)

[model]
	zstring name;     // "modelname" - used global model (in gamedata/models) | "&modelname" - used level's model (in %levelname%/geometry)
	... props

[entity]
	uint32  modelid;
	vec3    position;
	vec3    rotation;
	vec3    scale;

File structure:
	[header]
	[model]
	[model]
	...
	[entity]
	[entity]
	...

level.geom binary file:

[header]
	uint32 version;   // Format version
	uint32 compver;   // Compiler version
	uint32 objects;   // Phisycs objects

///// TMP /////
[physics]
	uint8  type;
	uint8  _offset;
	uint16 _offset;
	vec3   position;
	vec3   scale;

File structure:
[header]
[pm2]
[physics]

*/

namespace Engine {

}