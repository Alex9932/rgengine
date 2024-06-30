/*
 * rgEngine frustum.h
 *
 *  Created on: Jun 7, 2024
 *      Author: alex9932
 *
 */
#define DLL_EXPORT
#include "frustum.h"

namespace Engine {

	static RG_INLINE void NormalizePlane(vec4* plane) {
		Float32 len = SDL_sqrtf(plane->x * plane->x + plane->y * plane->y + plane->z * plane->z);
		plane->x /= len;
		plane->y /= len;
		plane->z /= len;
		plane->w /= len;
	}

	void CreateFrustum(CreateFrustumInfo* info) {

		mat4 clip_matrix = *info->proj * *info->view;
		float* clip = clip_matrix.m;

		info->result->planes[0].x = clip[3]  - clip[0];
		info->result->planes[0].y = clip[7]  - clip[4];
		info->result->planes[0].z = clip[11] - clip[8];
		info->result->planes[0].w = clip[15] - clip[12];
		info->result->planes[1].x = clip[3]  + clip[0];
		info->result->planes[1].y = clip[7]  + clip[4];
		info->result->planes[1].z = clip[11] + clip[8];
		info->result->planes[1].w = clip[15] + clip[12];
		info->result->planes[2].x = clip[3]  + clip[1];
		info->result->planes[2].y = clip[7]  + clip[5];
		info->result->planes[2].z = clip[11] + clip[9];
		info->result->planes[2].w = clip[15] + clip[13];
		info->result->planes[3].x = clip[3]  - clip[1];
		info->result->planes[3].y = clip[7]  - clip[5];
		info->result->planes[3].z = clip[11] - clip[9];
		info->result->planes[3].w = clip[15] - clip[13];
		info->result->planes[4].x = clip[3]  - clip[2];
		info->result->planes[4].y = clip[7]  - clip[6];
		info->result->planes[4].z = clip[11] - clip[10];
		info->result->planes[4].w = clip[15] - clip[14];
		info->result->planes[5].x = clip[3]  + clip[2];
		info->result->planes[5].y = clip[7]  + clip[6];
		info->result->planes[5].z = clip[11] + clip[10];
		info->result->planes[5].w = clip[15] + clip[14];

		// Normalize planes
		for (Uint32 i = 0; i < 6; i++) {
			NormalizePlane(&info->result->planes[i]);
		}

	}

	Bool SphereInFrustum(Frustum* f, const vec3& pos, Float32 r) {
		Bool result = true;
		for (Uint32 i = 0; i < 6; i++) {
			if (f->planes[i].x * pos.x + f->planes[i].y * pos.y + f->planes[i].z * pos.z + f->planes[i].w <= -r) { result = false; }
		}

		return result;
	}

	Bool AABBInFrustum(Frustum* f, AABB* aabb) {
		for (Uint32 i = 0; i < 6; i++) {
			if (!(f->planes[i].x * aabb->min.x + f->planes[i].y * aabb->min.y + f->planes[i].z * aabb->min.z + f->planes[i].w > 0.0f) &&
				!(f->planes[i].x * aabb->max.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f) &&
				!(f->planes[i].x * aabb->min.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->min.z + f->planes[i].w > 0.0f) &&
				!(f->planes[i].x * aabb->max.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->min.z + f->planes[i].w > 0.0f) &&
				!(f->planes[i].x * aabb->min.x + f->planes[i].y * aabb->min.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f) &&
				!(f->planes[i].x * aabb->max.x + f->planes[i].y * aabb->min.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f) &&
				!(f->planes[i].x * aabb->min.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f) &&
				!(f->planes[i].x * aabb->max.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f)) {
				return false;
			}
		}

		return true;
	}

	Bool AABBFullyInFrustum(Frustum* f, AABB* aabb) {
		Bool result = true;
		for (Uint32 i = 0; i < 6; i++) {
			if (!(f->planes[i].x * aabb->min.x + f->planes[i].y * aabb->min.y + f->planes[i].z * aabb->min.z + f->planes[i].w > 0.0f)) { result = false; }
			if (!(f->planes[i].x * aabb->max.x + f->planes[i].y * aabb->min.y + f->planes[i].z * aabb->min.z + f->planes[i].w > 0.0f)) { result = false; }
			if (!(f->planes[i].x * aabb->min.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->min.z + f->planes[i].w > 0.0f)) { result = false; }
			if (!(f->planes[i].x * aabb->max.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->min.z + f->planes[i].w > 0.0f)) { result = false; }
			if (!(f->planes[i].x * aabb->min.x + f->planes[i].y * aabb->min.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f)) { result = false; }
			if (!(f->planes[i].x * aabb->max.x + f->planes[i].y * aabb->min.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f)) { result = false; }
			if (!(f->planes[i].x * aabb->min.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f)) { result = false; }
			if (!(f->planes[i].x * aabb->max.x + f->planes[i].y * aabb->max.y + f->planes[i].z * aabb->max.z + f->planes[i].w > 0.0f)) { result = false; }
		}

		return result;
	}

	Bool PointInFrustum(Frustum* f, const vec3& p) {
		Bool result = true;
		for (Uint32 i = 0; i < 6; i++) {
			if (f->planes[i].x * p.x + f->planes[i].y * p.y + f->planes[i].z * p.z + f->planes[i].w <= 0.0f) { result = false; }
		}

		return result;
	}
}