#ifndef _RGMATH_H
#define _RGMATH_H

#include "rgtypes.h"
#include "rgvector.h"
#include "rgmatrix.h"

#if RG_SIMD
#include <xmmintrin.h>
#endif



typedef struct Frustum {
	union {
		vec4 planes[6];
		vec4 top, bottom, right, left, znear, zfar;
	};
} Frustum;

typedef struct AABB {
	vec3 min;
	vec3 max;
} AABB;

RG_DECLSPEC mat3 MAT3_IDENTITY();
RG_DECLSPEC mat4 MAT4_IDENTITY();

#if 0
RG_INLINE Uint32 rgCRC32(const void* buf, size_t len) { return rgCRC32((const char*)buf, (Uint32)len); }
RG_INLINE Uint32 rgCRC32(const void* buf, Uint32 len) { return rgCRC32((const char*)buf, len); }
RG_INLINE Uint32 rgCRC32(const char* buf, size_t len) { return rgCRC32(buf, (Uint32)len); }
#endif

RG_DECLSPEC Uint32 rgCRC32(const char* data, Uint32 len);

RG_DECLSPEC vec3 vec3_mulquat(const vec3& v, const quat& q);
RG_DECLSPEC void vec3_rotate(vec3* dst, const vec3& vector, const vec3& angles);
RG_DECLSPEC quat quat_axisAngle(const vec4& v);

RG_DECLSPEC Float32 rgRandFloat();

#define rgToRadians(x) (x * (RG_PI / 180.0f))
#define rgToDegrees(x) (x * (180.0f / RG_PI))

// API compatibility
#define rgToRadiansD rgToRadians
#define rgToDegreesD rgToDegrees

#define rgLerp(x, y, dt) ((x * dt) + (y * (1.0 - dt)))

#endif