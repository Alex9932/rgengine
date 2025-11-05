#ifndef __ASSIMPUTIL_H
#define __ASSIMPUTIL_H

#include <assimp/scene.h>
#include <rgmath.h>

static inline void CopyVector(vec3* dst, const aiVector3D& src) {
	dst->x = src.x;
	dst->y = src.y;
	dst->z = src.z;
}

static inline void CopyVector(quat* dst, const aiQuaternion& src) {
	dst->x = src.x;
	dst->y = src.y;
	dst->z = src.z;
	dst->w = src.w;
}

static inline void CopyMatrix(mat4* dst, const aiMatrix4x4& src) {
	dst->m00 = src.a1; dst->m01 = src.a2; dst->m02 = src.a3; dst->m03 = src.a4;
	dst->m10 = src.b1; dst->m11 = src.b2; dst->m12 = src.b3; dst->m13 = src.b4;
	dst->m20 = src.c1; dst->m21 = src.c2; dst->m22 = src.c3; dst->m23 = src.c4;
	dst->m30 = src.d1; dst->m31 = src.d2; dst->m32 = src.d3; dst->m33 = src.d4;
}

static inline Bool IsIdentity(const aiMatrix4x4& mat) {
	if (mat.a1 == 1 && mat.a2 == 0 && mat.a3 == 0 && mat.a4 == 0 &&
		mat.b1 == 0 && mat.b2 == 1 && mat.b3 == 0 && mat.b4 == 0 &&
		mat.c1 == 0 && mat.c2 == 0 && mat.c3 == 1 && mat.c4 == 0 &&
		mat.d1 == 0 && mat.d2 == 0 && mat.d3 == 0 && mat.d4 == 1) {
		return true;
	}
	return false;
}

#endif