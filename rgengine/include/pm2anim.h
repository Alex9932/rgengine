#ifndef _PM2ANIM_H
#define _PM2ANIM_H

#include "rgtypes.h"
#include "rgmath.h"

struct AnimHeader {
	char    magic[4];
	Float32 fps;
	Uint16  tracks;
	Uint16  version;
};

struct AnimTrack {
	char   name[32];
	Uint32 keyframes;
};

// TODO: Reorder fields?
struct AnimKeyframe {
	Float32 timestamp;
	quat    rotation;
	vec3    translation;
	vec3    scale;
	Sint8   interp[16];
};

#endif