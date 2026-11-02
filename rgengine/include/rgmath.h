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

	void Add(const vec3& v) {
		min += v;
		max += v;
	}
} AABB;

RG_DECLSPEC mat3 MAT3_IDENTITY();
RG_DECLSPEC mat4 MAT4_IDENTITY();

#if 0
RG_INLINE Uint32 rgCRC32(const void* buf, size_t len) { return rgCRC32((const char*)buf, (Uint32)len); }
RG_INLINE Uint32 rgCRC32(const void* buf, Uint32 len) { return rgCRC32((const char*)buf, len); }
RG_INLINE Uint32 rgCRC32(const char* buf, size_t len) { return rgCRC32(buf, (Uint32)len); }
#endif

RG_DECLSPEC Uint32 rgCRC32(const void* data, Uint32 len);
RG_DECLSPEC size_t rgHash(const void* data, size_t len);

RG_DECLSPEC vec3 vec3_mulquat(const vec3& v, const quat& q);
RG_DECLSPEC void vec3_rotate(vec3* dst, const vec3& vector, const vec3& angles);
RG_DECLSPEC quat quat_axisAngle(const vec4& v);

RG_DECLSPEC Float32 rgRandFloat();

#define rgToRadians(x) ((x) * (RG_PI / 180.0f))
#define rgToDegrees(x) ((x) * (180.0f / RG_PI))

// API compatibility
#define rgToRadiansD rgToRadians
#define rgToDegreesD rgToDegrees

#define rgLerp(x, y, dt) ((x * (1.0 - dt)) + (y * dt))

class RGPCG32 {
	public:
		RGPCG32(Uint64 seed = 0x853c49e6748fea9bULL, Uint64 seq = 0xda3e39cb94b95bdbULL) {
			m_state = 0;
			m_inc = (seq << 1u) | 1u;
			NextU32();
			m_state += seed;
			NextU32();
		}

		Uint32 NextU32() {
			Uint64 old = m_state;
			m_state = old * 6364136223846793005ULL + m_inc;
			Uint32 xorshifted = (Uint32)(((old >> 18u) ^ old) >> 27u);
			Uint32 rot = (Uint32)(old >> 59u);
			return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31u));
		}

		Uint64 NextU64() {
			Uint64 hi = NextU32();
			Uint64 lo = NextU32();
			return (hi << 32) | lo;
		}

	private:
		Uint64 m_state;
		Uint64 m_inc;
};

class RGRNG {
	public:
		RGRNG(Uint64 seed = 0x853c49e6748fea9bULL) : m_pcg(seed) {}

		Float32 UnitFloat() {
			return (m_pcg.NextU32() >> 8) * (1.0f / 16777216.0f);
		}

		Float64 UnitFloat64() {
			return (m_pcg.NextU64() >> 11) * (1.0 / 9007199254740992.0);
		}

		Float32 SignedFloat() { return UnitFloat() * 2.0f - 1.0f; }
		Float64 SignedFloat64() { return UnitFloat64() * 2.0 - 1.0; }

		Float32 RangeFloat(Float32 lo, Float32 hi) { return lo + UnitFloat() * (hi - lo); }
		Float64 RangeFloat64(Float64 lo, Float64 hi) { return lo + UnitFloat64() * (hi - lo); }

		Uint32 RangeU32(Uint32 lo, Uint32 hi) {
			Uint32 n = hi - lo + 1u;
			if (n == 0) { return m_pcg.NextU32(); }
			Uint32 threshold = (0u - n) % n;
			for (;;) {
				Uint32 r = m_pcg.NextU32();
				if (r >= threshold) return lo + (r % n);
			}
		}

		Sint32 RangeI32(Sint32 lo, Sint32 hi) {
			return (Sint32)(RangeU32(0u, ((Uint32)hi - (Uint32)lo))) + lo;
		}

	private:
		RGPCG32 m_pcg;
};

#endif