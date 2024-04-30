#ifndef _RGVECTOR_H
#define _RGVECTOR_H

#include "rgtypes.h"

union ivec2 {
    int array[2];
    struct {
        int x;
        int y;
    };
    struct {
        int r;
        int g;
    };
};

union ivec3 {
    int array[3];
    struct {
        int x;
        int y;
        int z;
    };
    struct {
        int r;
        int g;
        int b;
    };
};

union ivec4 {
    int array[4];
    struct {
        int x;
        int y;
        int z;
        int w;
    };
    struct {
        int r;
        int g;
        int b;
        int a;
    };
};

union vec2 {
    float array[2];
    struct {
        float x;
        float y;
    };

    RG_INLINE vec2 operator+(float a) {
        vec2 r;
        r.x = x + a;
        r.y = y + a;
        return r;
    }

    RG_INLINE vec2 operator+(const vec2& v) {
        vec2 r;
        r.x = x + v.x;
        r.y = y + v.y;
        return r;
    }

    RG_INLINE vec2 operator-(float a) {
        vec2 r;
        r.x = x - a;
        r.y = y - a;
        return r;
    }

    RG_INLINE vec2 operator-(const vec2& v) {
        vec2 r;
        r.x = x - v.x;
        r.y = y - v.y;
        return r;
    }

    RG_INLINE vec2 operator-() {
        vec2 r;
        r.x = -x;
        r.y = -y;
        return r;
    }

    RG_INLINE vec2 operator*(float a) {
        vec2 r;
        r.x = x * a;
        r.y = y * a;
        return r;
    }

    RG_INLINE vec2 operator*(const vec2& v) {
        vec2 r;
        r.x = x * v.x;
        r.y = y * v.y;
        return r;
    }

    RG_INLINE vec2 operator/(float a) {
        vec2 r;
        r.x = x / a;
        r.y = y / a;
        return r;
    }

    RG_INLINE vec2 operator/(const vec2& v) {
        vec2 r;
        r.x = x / v.x;
        r.y = y / v.y;
        return r;
    }

    RG_INLINE vec2 normalize() {
        float len = length();
        vec2 r;
        r.x = x / len;
        r.y = y / len;
        return r;
    }

    RG_INLINE float length() {
        return SDL_sqrtf(x * x + y * y);
    }

    RG_INLINE float dot(const vec2& v) {
        return x * v.x + y * v.y;
    }

    RG_INLINE float cross(const vec2& v) {
        return x * v.y + y * v.x;
    }

    RG_INLINE vec2 lerp(const vec2& v, float t) {
        float mt = 1.0f - t;
        vec2 r;
        r.x = x * mt + v.x * t;
        r.y = y * mt + v.y * t;
        return r;
    }

};

union vec3 {
    float array[3];
    struct {
        float x;
        float y;
        float z;
    };
    struct {
        float r;
        float g;
        float b;
    };

    RG_INLINE vec3 operator+(float a) {
        vec3 r;
        r.x = x + a;
        r.y = y + a;
        r.z = z + a;
        return r;
    }

    RG_INLINE vec3 operator+(const vec3& v) {
        vec3 r;
        r.x = x + v.x;
        r.y = y + v.y;
        r.z = z + v.z;
        return r;
    }

    RG_INLINE vec3 operator-(float a) {
        vec3 r;
        r.x = x - a;
        r.y = y - a;
        r.z = z - a;
        return r;
    }

    RG_INLINE vec3 operator-(const vec3& v) {
        vec3 r;
        r.x = x - v.x;
        r.y = y - v.y;
        r.z = z - v.z;
        return r;
    }

    RG_INLINE vec3 operator-() {
        vec3 r;
        r.x = -x;
        r.y = -y;
        r.z = -z;
        return r;
    }

    RG_INLINE vec3 operator*(float a) {
        vec3 r;
        r.x = x * a;
        r.y = y * a;
        r.z = z * a;
        return r;
    }

    RG_INLINE vec3 operator*(const vec3& v) {
        vec3 r;
        r.x = x * v.x;
        r.y = y * v.y;
        r.z = z * v.z;
        return r;
    }

    RG_INLINE vec3 operator/(float a) {
        vec3 r;
        r.x = x / a;
        r.y = y / a;
        r.z = z / a;
        return r;
    }

    RG_INLINE vec3 operator/(const vec3& v) {
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

    RG_INLINE vec3 normalize() {
        float len = length();
        vec3 r;
        r.x = x / len;
        r.y = y / len;
        r.z = z / len;
        return r;
    }

    RG_INLINE vec3 normalize_safe() {
        float len = length();
        vec3 r;
        if (len < 0.001f) {
            r.x = x;
            r.y = y;
            r.z = z;
            return r;
        }
        r.x = x / len;
        r.y = y / len;
        r.z = z / len;
        return r;
    }

    RG_INLINE float length() {
        return SDL_sqrtf(x * x + y * y + z * z);
    }

    RG_INLINE float dot(const vec3& v) {
        return x * v.x + y * v.y + z * v.z;
    }

    RG_INLINE vec3 cross(const vec3& v) {
        vec3 r;
        r.x = y * v.z - v.y * z;
        r.y = z * v.x - v.z * x;
        r.z = x * v.y - v.x * y;
        return r;
    }

    RG_INLINE vec3 lerp(const vec3& v, float t) {
        float mt = 1.0f - t;
        vec3 r;
        r.x = x * mt + v.x * t;
        r.y = y * mt + v.y * t;
        r.z = z * mt + v.z * t;
        return r;
    }

};

union vec4 {
    __m128 m;
    float array[4];
    struct {
        vec3 xyz;
        float w;
    };
    struct {
        vec3 rgb;
        float w;
    };
    struct {
        float x;
        float y;
        float z;
        float w;
    };
    struct {
        float r;
        float g;
        float b;
        float a;
    };

    //vec4() { x = 0; y = 0; z = 0; w = 0; }
    //vec4(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
    //vec4(vec3 v3, float _w) { x = v3.x; y = v3.y; z = v3.z; w = _w; }

    RG_INLINE vec4 operator+(float a) {
        vec4 r;
        r.x = x + a;
        r.y = y + a;
        r.z = z + a;
        r.w = w + a;
        return r;
    }

    RG_INLINE vec4 operator+(const vec4& v) {
        vec4 r;
        r.x = x + v.x;
        r.y = y + v.y;
        r.z = z + v.z;
        r.w = w + v.w;
        return r;
    }

    RG_INLINE vec4 operator-(float a) {
        vec4 r;
        r.x = x - a;
        r.y = y - a;
        r.z = z - a;
        r.w = w - a;
        return r;
    }

    RG_INLINE vec4 operator-(const vec4& v) {
        vec4 r;
        r.x = x - v.x;
        r.y = y - v.y;
        r.z = z - v.z;
        r.w = w - v.w;
        return r;
    }

    RG_INLINE vec4 operator-() {
        vec4 r;
        r.x = -x;
        r.y = -y;
        r.z = -z;
        r.w = w;
        return r;
    }

    RG_INLINE vec4 operator*(float a) {
        vec4 r;
        r.x = x * a;
        r.y = y * a;
        r.z = z * a;
        r.w = w * a;
        return r;
    }

    RG_INLINE vec4 operator*(const vec4& v) {
        vec4 r;
        r.x = x * v.x;
        r.y = y * v.y;
        r.z = z * v.z;
        r.w = w * v.w;
        return r;
    }

    RG_INLINE vec4 operator/(float a) {
        vec4 r;
        r.x = x / a;
        r.y = y / a;
        r.z = z / a;
        r.w = w / a;
        return r;
    }

    RG_INLINE vec4 operator/(const vec4& v) {
        vec4 r;
        r.x = x / v.x;
        r.y = y / v.y;
        r.z = z / v.z;
        r.w = w / v.w;
        return r;
    }

    //RG_INLINE vec4 operator=(vec4 v) {
    //    vec4 r;
    //    r.x = v.x;
    //    r.y = v.y;
    //    r.z = v.z;
    //    r.w = v.w;
    //    return r;
    //}

    RG_INLINE vec4 normalize() {
        float len = length();
        vec4 r;
        r.x = x / len;
        r.y = y / len;
        r.z = z / len;
        r.w = w / len;
        return r;
    }

    RG_INLINE float length() {
        return SDL_sqrtf(x * x + y * y + z * z + w * w);
    }

    RG_INLINE float dot(const vec4& v) {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    RG_INLINE vec4 lerp(const vec4& v, float t) {
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
    struct {
        float x;
        float y;
        float z;
        float w;
    };

    //quat() { x = 0; y = 0; z = 0; w = 0; }

    RG_INLINE quat operator*(const quat& q) {
        quat r;
        r.w = w * q.w - x * q.x - y * q.y - z * q.z;
        r.x = w * q.x + x * q.w + y * q.z - z * q.y;
        r.y = w * q.y + y * q.w + z * q.x - x * q.z;
        r.z = w * q.z + z * q.w + x * q.y - y * q.x;
        return r;
    }

    RG_INLINE quat slerp(const quat& q, float t) {
        quat r;
#if 0
        float dot = w * q.w + x * q.x + y * q.y + z * q.z;
        float theta = SDL_acosf(dot);
        if (theta < 0) {
            theta = -theta;
        }

        float st = SDL_sinf(theta);
        float sut = SDL_sinf(t * theta);
        float sout = SDL_sinf((1.0f - t) * theta);
        float c1 = sout / st;
        float c2 = sut / st;

        r.x = c1 * x + c2 * q.x;
        r.y = c1 * y + c2 * q.y;
        r.z = c1 * z + c2 * q.z;
        r.w = c1 * w + c2 * q.w;

        float len = SDL_sqrtf(r.x * r.x + r.y * r.y + r.z * r.z + r.w * r.w);
        r.x /= len;
        r.y /= len;
        r.z /= len;
#else
        float dot = w * q.w + x * q.x + y * q.y + z * q.z;
        float blendI = 1.0f - t;
        if (dot < 0) {
            r.w = blendI * w + t * -q.w;
            r.x = blendI * x + t * -q.x;
            r.y = blendI * y + t * -q.y;
            r.z = blendI * z + t * -q.z;
        }
        else {
            r.w = blendI * w + t * q.w;
            r.x = blendI * x + t * q.x;
            r.y = blendI * y + t * q.y;
            r.z = blendI * z + t * q.z;
        }
#endif
        return r;
    }

    RG_INLINE vec3 toEuler() {
        vec3 r;
        float sqw = w * w;
        float sqx = x * x;
        float sqy = y * y;
        float sqz = z * z;
        float unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
        float test = x * y + z * w;
        if (test > 0.499 * unit) { // singularity at north pole
            float heading = 2 * SDL_atan2f(x, w);
            float attitude = RG_PI / 2;
            float bank = 0;
            r.x = attitude;
            r.y = heading;
            r.z = bank;
            return r;
        }
        if (test < -0.499 * unit) { // singularity at south pole
            float heading = -2 * SDL_atan2f(x, w);
            float attitude = -RG_PI / 2;
            float bank = 0;
            r.x = attitude;
            r.y = heading;
            r.z = bank;
            return r;
        }
        float heading = SDL_atan2f(2 * y * w - 2 * x * z, sqx - sqy - sqz + sqw);
        float attitude = SDL_asinf(2 * test / unit);
        float bank = SDL_atan2f(2 * x * w - 2 * y * z, -sqx + sqy - sqz + sqw);
        r.x = attitude;
        r.y = heading;
        r.z = bank;
        return r;
    }

    RG_INLINE quat conjugate() {
        quat r;
        r.x = -x;
        r.y = -y;
        r.z = -z;
        r.w = w;
        return r;
    }
};

#endif