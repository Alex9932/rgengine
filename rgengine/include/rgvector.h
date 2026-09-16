#ifndef _RGVECTOR_H
#define _RGVECTOR_H

#include "rgtypes.h"

union ivec2 {
    int array[2];
    struct { int x; int y; };
    struct { int r; int g; };
};

union ivec3 {
    int array[3];
    struct { int x; int y; int z; };
    struct { int r; int g; int b; };
};

union ivec4 {
    int array[4];
    struct { int x; int y; int z; int w; };
    struct { int r; int g; int b; int a; };
};

union vec2 {
    float array[2];
    struct { float x; float y; };

    RG_INLINE vec2 operator+(float a) const {
        vec2 r;
        r.x = x + a;
        r.y = y + a;
        return r;
    }

    RG_INLINE vec2 operator+(const vec2& v) const {
        vec2 r;
        r.x = x + v.x;
        r.y = y + v.y;
        return r;
    }

    RG_INLINE vec2 operator-(float a) const {
        vec2 r;
        r.x = x - a;
        r.y = y - a;
        return r;
    }

    RG_INLINE vec2 operator-(const vec2& v) const {
        vec2 r;
        r.x = x - v.x;
        r.y = y - v.y;
        return r;
    }

    RG_INLINE vec2 operator-() const {
        vec2 r;
        r.x = -x;
        r.y = -y;
        return r;
    }

    RG_INLINE vec2 operator*(float a) const {
        vec2 r;
        r.x = x * a;
        r.y = y * a;
        return r;
    }

    RG_INLINE vec2 operator*(const vec2& v) const {
        vec2 r;
        r.x = x * v.x;
        r.y = y * v.y;
        return r;
    }

    RG_INLINE vec2 operator/(float a) const {
        vec2 r;
        r.x = x / a;
        r.y = y / a;
        return r;
    }

    RG_INLINE vec2 operator/(const vec2& v) const {
        vec2 r;
        r.x = x / v.x;
        r.y = y / v.y;
        return r;
    }

    RG_INLINE vec2 normalize() const {
        float len = length();
        vec2 r;
        r.x = x / len;
        r.y = y / len;
        return r;
    }

    RG_INLINE float length() const {
        return SDL_sqrtf(x * x + y * y);
    }

    RG_INLINE float dot(const vec2& v) const {
        return x * v.x + y * v.y;
    }

    RG_INLINE float cross(const vec2& v) const {
        return x * v.y - y * v.x;
    }

    RG_INLINE vec2 lerp(const vec2& v, float t) const {
        float mt = 1.0f - t;
        vec2 r;
        r.x = x * mt + v.x * t;
        r.y = y * mt + v.y * t;
        return r;
    }

    RG_INLINE Bool operator!=(const vec2& v) {
        return x != v.x || y != v.y;
    }

};

union vec3 {
    float array[3];
    struct { float x; float y; float z; };
    struct { float r; float g; float b; };

    RG_INLINE vec3 operator+(float a) const {
        vec3 r;
        r.x = x + a;
        r.y = y + a;
        r.z = z + a;
        return r;
    }

    RG_INLINE vec3 operator+(const vec3& v) const {
        vec3 r;
        r.x = x + v.x;
        r.y = y + v.y;
        r.z = z + v.z;
        return r;
    }

    RG_INLINE vec3 operator-(float a) const {
        vec3 r;
        r.x = x - a;
        r.y = y - a;
        r.z = z - a;
        return r;
    }

    RG_INLINE vec3 operator-(const vec3& v) const {
        vec3 r;
        r.x = x - v.x;
        r.y = y - v.y;
        r.z = z - v.z;
        return r;
    }

    RG_INLINE vec3 operator-() const {
        vec3 r;
        r.x = -x;
        r.y = -y;
        r.z = -z;
        return r;
    }

    RG_INLINE vec3 operator*(float a) const {
        vec3 r;
        r.x = x * a;
        r.y = y * a;
        r.z = z * a;
        return r;
    }

    RG_INLINE vec3 operator*(const vec3& v) const {
        vec3 r;
        r.x = x * v.x;
        r.y = y * v.y;
        r.z = z * v.z;
        return r;
    }

    RG_INLINE vec3 operator/(float a) const {
        vec3 r;
        r.x = x / a;
        r.y = y / a;
        r.z = z / a;
        return r;
    }

    RG_INLINE vec3 operator/(const vec3& v) const {
        vec3 r;
        r.x = x / v.x;
        r.y = y / v.y;
        r.z = z / v.z;
        return r;
    }

    RG_INLINE void operator+=(float a) {
        x += a;
        y += a;
        z += a;
    }

    RG_INLINE void operator+=(const vec3& v) {
        x += v.x;
        y += v.y;
        z += v.z;
    }

    RG_INLINE void operator-=(float a) {
        x -= a;
        y -= a;
        z -= a;
    }

    RG_INLINE void operator-=(const vec3& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
    }

    RG_INLINE void operator*=(float a) {
        x *= a;
        y *= a;
        z *= a;
    }

    RG_INLINE vec3 normalize() const {
        float len = length();
        vec3 r;
        r.x = x / len;
        r.y = y / len;
        r.z = z / len;
        return r;
    }

    RG_INLINE vec3 normalize_safe() const {
        float len = length();
        vec3 r;
        if (len < 0.000001f) {
            r.x = 0;
            r.y = 0;
            r.z = 0;
            return r;
        }
        r.x = x / len;
        r.y = y / len;
        r.z = z / len;
        return r;
    }

    RG_INLINE float length() const {
        return SDL_sqrtf(x * x + y * y + z * z);
    }

    RG_INLINE float dot(const vec3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    RG_INLINE vec3 cross(const vec3& v) const {
        vec3 r;
        r.x = y * v.z - v.y * z;
        r.y = z * v.x - v.z * x;
        r.z = x * v.y - v.x * y;
        return r;
    }

    RG_INLINE vec3 lerp(const vec3& v, float t) const {
        float mt = 1.0f - t;
        vec3 r;
        r.x = x * mt + v.x * t;
        r.y = y * mt + v.y * t;
        r.z = z * mt + v.z * t;
        return r;
    }

    RG_INLINE vec3 lerp(const vec3& v, const vec3& t) const {
        float mtx = 1.0f - t.x;
        float mty = 1.0f - t.y;
        float mtz = 1.0f - t.z;
        vec3 r;
        r.x = x * mtx + v.x * t.x;
        r.y = y * mty + v.y * t.y;
        r.z = z * mtz + v.z * t.z;
        return r;
    }

    RG_INLINE Bool operator!=(const vec3& v) {
        return x != v.x || y != v.y || z != v.z;
    }

    RG_INLINE Bool operator==(const vec3& v) {
        return x == v.x && y == v.y && z == v.z;
    }

};

union vec4 {
#if RG_SIMD
    __m128 m;
#endif
    float array[4];
    struct { vec3 xyz; float w; };
    struct { vec3 rgb; float a; };
    struct { float x; float y; float z; float w; };
    struct { float r; float g; float b; float a; };

    RG_INLINE vec4 operator+(float a) const {
        vec4 r;
        r.x = x + a;
        r.y = y + a;
        r.z = z + a;
        r.w = w + a;
        return r;
    }

    RG_INLINE vec4 operator+(const vec4& v) const {
        vec4 r;
        r.x = x + v.x;
        r.y = y + v.y;
        r.z = z + v.z;
        r.w = w + v.w;
        return r;
    }

    RG_INLINE vec4 operator-(float a) const {
        vec4 r;
        r.x = x - a;
        r.y = y - a;
        r.z = z - a;
        r.w = w - a;
        return r;
    }

    RG_INLINE vec4 operator-(const vec4& v) const{
        vec4 r;
        r.x = x - v.x;
        r.y = y - v.y;
        r.z = z - v.z;
        r.w = w - v.w;
        return r;
    }

    RG_INLINE vec4 operator-() const {
        vec4 r;
        r.x = -x;
        r.y = -y;
        r.z = -z;
        r.w = -w;
        return r;
    }

    RG_INLINE vec4 operator*(float a) const {
        vec4 r;
        r.x = x * a;
        r.y = y * a;
        r.z = z * a;
        r.w = w * a;
        return r;
    }

    RG_INLINE vec4 operator*(const vec4& v) const {
        vec4 r;
        r.x = x * v.x;
        r.y = y * v.y;
        r.z = z * v.z;
        r.w = w * v.w;
        return r;
    }

    RG_INLINE vec4 operator/(float a) const {
        vec4 r;
        r.x = x / a;
        r.y = y / a;
        r.z = z / a;
        r.w = w / a;
        return r;
    }

    RG_INLINE vec4 operator/(const vec4& v) const {
        vec4 r;
        r.x = x / v.x;
        r.y = y / v.y;
        r.z = z / v.z;
        r.w = w / v.w;
        return r;
    }

    RG_INLINE vec4 normalize() const {
        float len = length();
        vec4 r;
        r.x = x / len;
        r.y = y / len;
        r.z = z / len;
        r.w = w / len;
        return r;
    }

    RG_INLINE float length() const {
        return SDL_sqrtf(x * x + y * y + z * z + w * w);
    }

    RG_INLINE float dot(const vec4& v) const {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    RG_INLINE vec4 lerp(const vec4& v, float t) const {
        float mt = 1.0f - t;
        vec4 r;
        r.x = x * mt + v.x * t;
        r.y = y * mt + v.y * t;
        r.z = z * mt + v.z * t;
        r.w = w * mt + v.w * t;
        return r;
    }

};

union quat {
    vec4 v4;
    struct { float x; float y; float z; float w; };

    RG_INLINE quat operator*(const quat& q) const {
        quat r;
        r.w = w * q.w - x * q.x - y * q.y - z * q.z;
        r.x = w * q.x + x * q.w + y * q.z - z * q.y;
        r.y = w * q.y + y * q.w + z * q.x - x * q.z;
        r.z = w * q.z + z * q.w + x * q.y - y * q.x;
        return r;
    }

    RG_DECLSPEC quat nlerp(const quat& q, float t) const;
    RG_DECLSPEC quat slerp(const quat& q, float t) const;
    RG_DECLSPEC vec3 toEuler() const;

    RG_INLINE quat conjugate() const {
        quat r;
        r.x = -x;
        r.y = -y;
        r.z = -z;
        r.w = w;
        return r;
    }

    RG_INLINE quat normalize() const {
        float len = length();
        quat r;
        r.x = x / len;
        r.y = y / len;
        r.z = z / len;
        r.w = w / len;
        return r;
    }

    RG_INLINE float length() const {
        return SDL_sqrtf(x * x + y * y + z * z + w * w);
    }
};

#endif