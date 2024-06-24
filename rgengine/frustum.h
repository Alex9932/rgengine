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
	Float32  znear;
	Float32  zfar;
	Float32  fov;
	Float32  aspect;
	vec3     position;
	vec3     rotation;
} CreateFrustumInfo;

namespace Engine {

	RG_DECLSPEC void CreateFrustum(CreateFrustumInfo* info);

	RG_DECLSPEC Bool SphereInFrustum(Frustum* f, const vec3& pos, Float32 r);
	RG_DECLSPEC Bool AABBInFrustum(Frustum* f, const AABB& aabb);

}

#endif