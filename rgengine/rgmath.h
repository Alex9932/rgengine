#ifndef _RGMATH_H
#define  RGMATH_H

#include "rgtypes.h"
#include "rgvector.h"
#include "rgmatrix.h"

#if RG_SIMD
#include <xmmintrin.h>
#endif

RG_DECLSPEC mat3 MAT3_IDENTITY();
RG_DECLSPEC mat4 MAT4_IDENTITY();

RG_DECLSPEC Uint32 rgCRC32(const char* data, Uint32 len);

RG_DECLSPEC vec3 vec3_mulquat(const vec3& v, const quat& q);
RG_DECLSPEC quat quat_axisAngle(const vec4& v);

#define rgToRadians(x) (x * (RG_PI / 180.0f))
#define rgToDegrees(x) (x * (180.0f / RG_PI))

// API compatibility
#define rgToRadiansD rgToRadians
#define rgToDegreesD rgToDegrees

#define rgLerp(x, y, dt) ((x * dt) + (y * (1.0 - dt)))

#endif