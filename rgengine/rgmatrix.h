#ifndef _RGMATRIX_H
#define _RGMATRIX_H

#include "rgvector.h"

union mat3 {
    float m[9];
    float m2[3][3];
    vec3 cols[3];

    RG_INLINE vec3 operator*(const vec3& v) const {
        vec3 r;
        r.x = v.x * m[0] + v.y * m[3] + v.z * m[6];
        r.y = v.x * m[1] + v.y * m[4] + v.z * m[7];
        r.z = v.x * m[2] + v.y * m[5] + v.z * m[8];
        return r;
    }

    // TODO: implement or delete (mat3 is rarely used)
    RG_INLINE mat3 operator*(const mat3& mat) const {
        mat3 r = {};
        return r;
    }
};

/*
    |OpenGL matrix 4x4|
    | m00 m01 m02 m03 |
    | m10 m11 m12 m13 |
    | m20 m21 m22 m23 |
    | m30 m31 m32 m33 |
*/
union mat4 {
    struct {
        float m00; float m10; float m20; float m30;
        float m01; float m11; float m21; float m31;
        float m02; float m12; float m22; float m32;
        float m03; float m13; float m23; float m33;
    };
    float m[16];
    float m2[4][4];
    vec4 cols[4];

    RG_INLINE vec4 operator*(const vec4& v) const {
        vec4 r;
#if RG_SIMD
        __m128 vX = _mm_shuffle_ps(v.m, v.m, 0x00);
        __m128 vY = _mm_shuffle_ps(v.m, v.m, 0x55);
        __m128 vZ = _mm_shuffle_ps(v.m, v.m, 0xAA);
        __m128 vW = _mm_shuffle_ps(v.m, v.m, 0xFF);
        __m128 _r = _mm_mul_ps(vX, this->cols[0].m);
        _r = _mm_add_ps(_r, _mm_mul_ps(vY, this->cols[1].m));
        _r = _mm_add_ps(_r, _mm_mul_ps(vZ, this->cols[2].m));
        _r = _mm_add_ps(_r, _mm_mul_ps(vW, this->cols[3].m));
        r.m = _r;
#else
        r.x = v.x * m[0] + v.y * m[4] + v.z * m[8] + v.w * m[12];
        r.y = v.x * m[1] + v.y * m[5] + v.z * m[9] + v.w * m[13];
        r.z = v.x * m[2] + v.y * m[6] + v.z * m[10] + v.w * m[14];
        r.w = v.x * m[3] + v.y * m[7] + v.z * m[11] + v.w * m[15];
#endif
        return r;
    }

    RG_INLINE vec3 operator*(const vec3& v) const {
        vec4 v4 = {v.x, v.y, v.z, 1};
        vec4 r4 = *this * v4;
        vec3 r  = {r4.x, r4.y, r4.z};
        return r;
    }

    RG_INLINE mat4 operator*(const mat4& mat) const {
        mat4 r;
        r.cols[0] = *this * mat.cols[0];
        r.cols[1] = *this * mat.cols[1];
        r.cols[2] = *this * mat.cols[2];
        r.cols[3] = *this * mat.cols[3];
        return r;
    }
};

RG_DECLSPEC void mat4_ortho(mat4* dst, float l, float r, float b, float t, float n, float f);
RG_DECLSPEC void mat4_frustum(mat4* dst, float fov, float aspect, float znear, float zfar);

RG_DECLSPEC void mat4_view(mat4* dst, const vec3& pos, const vec3& rot);
RG_DECLSPEC void mat4_model(mat4* dst, const vec3& pos, const vec3& rot, const vec3& scale);
RG_DECLSPEC void mat4_modelQ(mat4* dst, const vec3& pos, const quat& rot, const vec3& scale);

RG_DECLSPEC void mat4_transpose(mat4* dst, const mat4& src);
RG_DECLSPEC void mat4_rotatex(mat4* mat, float angle);
RG_DECLSPEC void mat4_rotatey(mat4* mat, float angle);
RG_DECLSPEC void mat4_rotatez(mat4* mat, float angle);
RG_DECLSPEC void mat4_rotate(mat4* mat, const vec3& angles);
RG_DECLSPEC void mat4_translate(mat4* mat, const vec3& pos);
RG_DECLSPEC void mat4_fromquat(mat4* mat, const quat& q);
RG_DECLSPEC void mat4_inverse(mat4* dst, const mat4& src);
RG_DECLSPEC float mat4_determinant(const mat4& m);
RG_DECLSPEC void mat4_decompose(vec3* position, quat* quaternion, vec3* scale, const mat4& matrix);
RG_DECLSPEC void mat4ToQuat(quat* q, const mat4& m);

#endif 