/*
 * rgEngine frustum.h
 *
 *  Created on: Jun 7, 2024
 *      Author: alex9932
 *
 */

#ifndef _FRUSTUM_H
#define _FRUSTUM_H

#include "rgmath.h"

typedef struct CreateFrustumInfo {
	Frustum* result;
	mat4*    proj;
	mat4*    view;
} CreateFrustumInfo;

namespace Engine {

	RG_DECLSPEC void CreateFrustum(CreateFrustumInfo* info);

	RG_DECLSPEC Bool SphereInFrustum(Frustum* f, const vec3& pos, Float32 r);
	RG_DECLSPEC Bool AABBInFrustum(Frustum* f, AABB* aabb);
	RG_DECLSPEC Bool AABBFullyInFrustum(Frustum* f, AABB* aabb);

}

#endif