#define DLL_EXPORT
#include "rgmatrix.h"

#undef far
#undef near

void mat4_ortho(mat4* mat, float l, float r, float b, float t, float n, float f) {
    mat->m00 = 2 / (r - l); mat->m01 = 0;           mat->m02 = 0;            mat->m03 = -((r + l) / (r - l));
    mat->m10 = 0;           mat->m11 = 2 / (t - b); mat->m12 = 0;            mat->m13 = -((t + b) / (t - b));
    mat->m20 = 0;           mat->m21 = 0;           mat->m22 = -2 / (f - n); mat->m23 = -((f + n) / (f - n));
    mat->m30 = 0;           mat->m31 = 0;           mat->m32 = 0;            mat->m33 = 1;
}

void mat4_frustum(mat4* mat, float fov, float aspect, float near, float far) {
    float tg = SDL_tanf(fov / 2);
    float m00 = 1.0f / (aspect * tg);
    float m11 = 1.0f / tg;
    float far_near = far - near;
    float a = -(far + near) / (far_near);
    float b = -(2 * far * near) / (far_near);
    mat->m00 = m00; mat->m01 = 0;   mat->m02 = 0; mat->m03 = 0;
    mat->m10 = 0;   mat->m11 = m11; mat->m12 = 0; mat->m13 = 0;
    mat->m20 = 0;   mat->m21 = 0;   mat->m22 = a; mat->m23 = b;
    mat->m30 = 0;   mat->m31 = 0;   mat->m32 = -1; mat->m33 = 0;
}

void mat4_view(mat4* dst, const vec3& pos, const vec3& rot) {
    mat4 m_translate;
    mat4 m_rotate;
    vec3 invPos = { -pos.x, -pos.y, -pos.z };
    mat4_rotate(&m_rotate, rot);
    mat4_translate(&m_translate, invPos);
    *dst = m_rotate * m_translate;
}

void mat4_model(mat4* dst, const vec3& pos, const vec3& rot, const vec3& scale) {
    mat4 m_translate;
    mat4 m_rotate;
    mat4 m_scale = {
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        0, 0, 0, 1
    };
    mat4_rotate(&m_rotate, rot);
    mat4_translate(&m_translate, pos);
    *dst = m_translate * m_rotate * m_scale;
}

void mat4_transpose(mat4* dst, const mat4& src) {
    dst->m00 = src.m00; dst->m10 = src.m01; dst->m20 = src.m02; dst->m30 = src.m03;
    dst->m01 = src.m10; dst->m11 = src.m11; dst->m21 = src.m12; dst->m31 = src.m13;
    dst->m02 = src.m20; dst->m12 = src.m21; dst->m22 = src.m22; dst->m32 = src.m23;
    dst->m03 = src.m30; dst->m13 = src.m31; dst->m23 = src.m32; dst->m33 = src.m33;
}

void mat4_rotatex(mat4* mat, float angle) {
    float s = SDL_sinf(angle);
    float c = SDL_cosf(angle);
    mat->m00 = 1; mat->m01 = 0; mat->m02 = 0;  mat->m03 = 0;
    mat->m10 = 0; mat->m11 = c; mat->m12 = -s; mat->m13 = 0;
    mat->m20 = 0; mat->m21 = s; mat->m22 = c; mat->m23 = 0;
    mat->m30 = 0; mat->m31 = 0; mat->m32 = 0;  mat->m33 = 1;
}

void mat4_rotatey(mat4* mat, float angle) {
    float s = SDL_sinf(angle);
    float c = SDL_cosf(angle);
    mat->m00 = c; mat->m01 = 0; mat->m02 = s; mat->m03 = 0;
    mat->m10 = 0; mat->m11 = 1; mat->m12 = 0; mat->m13 = 0;
    mat->m20 = -s; mat->m21 = 0; mat->m22 = c; mat->m23 = 0;
    mat->m30 = 0; mat->m31 = 0; mat->m32 = 0; mat->m33 = 1;
}

void mat4_rotatez(mat4* mat, float angle) {
    float s = SDL_sinf(angle);
    float c = SDL_cosf(angle);
    mat->m00 = c; mat->m01 = -s; mat->m02 = 0; mat->m03 = 0;
    mat->m10 = s; mat->m11 = c; mat->m12 = 0; mat->m13 = 0;
    mat->m20 = 0; mat->m21 = 0; mat->m22 = 1; mat->m23 = 0;
    mat->m30 = 0; mat->m31 = 0; mat->m32 = 0; mat->m33 = 1;
}

// x - yaw, y - pitch, z - roll
void mat4_rotate(mat4* mat, const vec3& angles) {
    //float sinx = SDL_sinf(angles.x);
    //float cosx = SDL_cosf(angles.x);
    //float siny = SDL_sinf(angles.y);
    //float cosy = SDL_cosf(angles.y);
    //float sinz = SDL_sinf(angles.z);
    //float cosz = SDL_cosf(angles.z);
    //mat->m00 =  cosy*cosz; mat->m10 = sinx*siny*cosz - cosx*sinz; mat->m20 = cosx*siny*cosz + sinx*sinz; mat->m30 = 0;
    //mat->m01 =  cosy*sinz; mat->m11 = sinx*siny*sinz + cosx*cosz; mat->m21 = cosx*siny*sinz - sinx*cosz; mat->m31 = 0;
    //mat->m02 = -siny;      mat->m12 = sinx*cosy;                  mat->m22 = cosx*cosy;                  mat->m32 = 0;
    //mat->m03 =  0;         mat->m13 = 0;                          mat->m23 = 0;                          mat->m33 = 1;

    mat4 rx, ry, rz, ryz;
    mat4_rotatex(&rx, angles.x);
    mat4_rotatey(&ry, angles.y);
    mat4_rotatez(&rz, angles.z);
    ryz = rz * ry;
    *mat = rx * ryz;
}

void mat4_translate(mat4* mat, const vec3& pos) {
    mat->m00 = 1; mat->m01 = 0; mat->m02 = 0; mat->m03 = pos.x;
    mat->m10 = 0; mat->m11 = 1; mat->m12 = 0; mat->m13 = pos.y;
    mat->m20 = 0; mat->m21 = 0; mat->m22 = 1; mat->m23 = pos.z;
    mat->m30 = 0; mat->m31 = 0; mat->m32 = 0; mat->m33 = 1;
}

void mat4_fromquat(mat4* mat, const quat& q) {
    mat->m00 = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    mat->m10 = 2.0f * (q.x * q.y + q.w * q.z);
    mat->m20 = 2.0f * (q.x * q.z - q.w * q.y);
    mat->m30 = 0.0f;
    mat->m01 = 2.0f * (q.x * q.y - q.w * q.z);
    mat->m11 = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
    mat->m21 = 2.0f * (q.y * q.z + q.w * q.x);
    mat->m31 = 0.0f;
    mat->m02 = 2.0f * (q.x * q.z + q.w * q.y);
    mat->m12 = 2.0f * (q.y * q.z - q.w * q.x);
    mat->m22 = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    mat->m32 = 0.0f;
    mat->m03 = 0.0f;
    mat->m13 = 0.0f;
    mat->m23 = 0.0f;
    mat->m33 = 1.0f;
}

void mat4_inverse(mat4* dst, const mat4& src) {
    float* m = (float*)src.m;
    float* _dest = (float*)dst;
    float inv[16];
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    float det = 1.0 / (m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12]);

    for (size_t i = 0; i < 16; i++) {
        _dest[i] = inv[i] * det;
    }
}

float mat4_determinant(const mat4& m) {
    return
        m.m03 * m.m12 * m.m21 * m.m30 - m.m02 * m.m13 * m.m21 * m.m30 - m.m03 * m.m11 * m.m22 * m.m30 + m.m01 * m.m13 * m.m22 * m.m30 +
        m.m02 * m.m11 * m.m23 * m.m30 - m.m01 * m.m12 * m.m23 * m.m30 - m.m03 * m.m12 * m.m20 * m.m31 + m.m02 * m.m13 * m.m20 * m.m31 +
        m.m03 * m.m10 * m.m22 * m.m31 - m.m00 * m.m13 * m.m22 * m.m31 - m.m02 * m.m10 * m.m23 * m.m31 + m.m00 * m.m12 * m.m23 * m.m31 +
        m.m03 * m.m11 * m.m20 * m.m32 - m.m01 * m.m13 * m.m20 * m.m32 - m.m03 * m.m10 * m.m21 * m.m32 + m.m00 * m.m13 * m.m21 * m.m32 +
        m.m01 * m.m10 * m.m23 * m.m32 - m.m00 * m.m11 * m.m23 * m.m32 - m.m02 * m.m11 * m.m20 * m.m33 + m.m01 * m.m12 * m.m20 * m.m33 +
        m.m02 * m.m10 * m.m21 * m.m33 - m.m00 * m.m12 * m.m21 * m.m33 - m.m01 * m.m10 * m.m22 * m.m33 + m.m00 * m.m11 * m.m22 * m.m33;
}

void mat4_decompose(vec3* position, quat* quaternion, vec3* scale, const mat4& matrix) {
    vec3 _x = { matrix.m00, matrix.m10, matrix.m20 };
    vec3 _y = { matrix.m01, matrix.m11, matrix.m21 };
    vec3 _z = { matrix.m02, matrix.m12, matrix.m22 };
    float sx = _x.length();
    float sy = _y.length();
    float sz = _z.length();

    float det = mat4_determinant(matrix);

    if (det < 0) { sx = -sx; }

    position->x = matrix.m03;
    position->y = matrix.m13;
    position->z = matrix.m23;

    mat4 _m = matrix;

    float invSX = 1.0 / sx;
    float invSY = 1.0 / sy;
    float invSZ = 1.0 / sz;

    _m.m00 *= invSX;
    _m.m10 *= invSX;
    _m.m20 *= invSX;

    _m.m01 *= invSY;
    _m.m11 *= invSY;
    _m.m21 *= invSY;

    _m.m02 *= invSZ;
    _m.m12 *= invSZ;
    _m.m22 *= invSZ;

    mat4ToQuat(quaternion, _m);

    scale->x = sx;
    scale->y = sy;
    scale->z = sz;
}

void mat4ToQuat(quat* dest, const mat4& matrix) {

    float w, x, y, z;
    float diagonal = matrix.m00 + matrix.m11 + matrix.m22;

    if (diagonal > 0) {
        float w4 = (float)(SDL_sqrtf(diagonal + 1) * 2);
        w = w4 / 4;
        x = (matrix.m21 - matrix.m12) / w4;
        y = (matrix.m02 - matrix.m20) / w4;
        z = (matrix.m10 - matrix.m01) / w4;
    }
    else if ((matrix.m00 > matrix.m11) && (matrix.m00 > matrix.m22)) {
        float x4 = (float)(SDL_sqrtf(1 + matrix.m00 - matrix.m11 - matrix.m22) * 2);
        w = (matrix.m21 - matrix.m12) / x4;
        x = x4 / 4;
        y = (matrix.m01 + matrix.m10) / x4;
        z = (matrix.m02 + matrix.m20) / x4;
    }
    else if (matrix.m11 > matrix.m22) {
        float y4 = (float)(SDL_sqrtf(1 + matrix.m11 - matrix.m00 - matrix.m22) * 2);
        w = (matrix.m02 - matrix.m20) / y4;
        x = (matrix.m01 + matrix.m10) / y4;
        y = y4 / 4;
        z = (matrix.m12 + matrix.m21) / y4;
    }
    else {
        float z4 = (float)(SDL_sqrtf(1 + matrix.m22 - matrix.m00 - matrix.m11) * 2);
        w = (matrix.m10 - matrix.m01) / z4;
        x = (matrix.m02 + matrix.m20) / z4;
        y = (matrix.m12 + matrix.m21) / z4;
        z = z4 / 4;
    }

    dest->x = x;
    dest->y = y;
    dest->z = z;
    dest->w = w;

#if 0
    float w, x, y, z;
    float diagonal = matrix.m00 + matrix.m11 + matrix.m22;

    if (diagonal > 0) {
        float w4 = (float)(SDL_sqrtf(diagonal + 1) * 2);
        w = w4 / 4;
        x = (matrix.m21 - matrix.m12) / w4;
        y = (matrix.m02 - matrix.m20) / w4;
        z = (matrix.m10 - matrix.m01) / w4;
    }
    else if ((matrix.m00 > matrix.m11) && (matrix.m00 > matrix.m22)) {
        float x4 = (float)(SDL_sqrtf(1 + matrix.m00 - matrix.m11 - matrix.m22) * 2);
        w = (matrix.m21 - matrix.m12) / x4;
        x = x4 / 4;
        y = (matrix.m01 + matrix.m10) / x4;
        z = (matrix.m02 + matrix.m20) / x4;
    }
    else if (matrix.m11 > matrix.m22) {
        float y4 = (float)(SDL_sqrtf(1 + matrix.m11 - matrix.m00 - matrix.m22) * 2);
        w = (matrix.m02 - matrix.m20) / y4;
        x = (matrix.m01 + matrix.m10) / y4;
        y = y4 / 4;
        z = (matrix.m12 + matrix.m21) / y4;
    }
    else {
        float z4 = (float)(SDL_sqrtf(1 + matrix.m22 - matrix.m00 - matrix.m11) * 2);
        w = (matrix.m10 - matrix.m01) / z4;
        x = (matrix.m02 + matrix.m20) / z4;
        y = (matrix.m12 + matrix.m21) / z4;
        z = z4 / 4;
    }

    dest->x = x;
    dest->y = y;
    dest->z = z;
    dest->w = w;
#endif

#if 0
    float w, x, y, z;
    float diagonal = matrix.m00 + matrix.m11 + matrix.m22;

    if (diagonal > 0) {
        float w4 = (float)(SDL_sqrtf(diagonal + 1) * 2);
        w = w4 / 4;
        x = (matrix.m21 - matrix.m12) / w4;
        y = (matrix.m02 - matrix.m20) / w4;
        z = (matrix.m10 - matrix.m01) / w4;
    }
    else if ((matrix.m00 > matrix.m11) && (matrix.m00 > matrix.m22)) {
        float x4 = (float)(SDL_sqrtf(1 + matrix.m00 - matrix.m11 - matrix.m22) * 2);
        w = (matrix.m21 - matrix.m12) / x4;
        x = x4 / 4;
        y = (matrix.m01 + matrix.m10) / x4;
        z = (matrix.m02 + matrix.m20) / x4;
    }
    else if (matrix.m11 > matrix.m22) {
        float y4 = (float)(SDL_sqrtf(1 + matrix.m11 - matrix.m00 - matrix.m22) * 2);
        w = (matrix.m02 - matrix.m20) / y4;
        x = (matrix.m01 + matrix.m10) / y4;
        y = y4 / 4;
        z = (matrix.m12 + matrix.m21) / y4;
    }
    else {
        float z4 = (float)(SDL_sqrtf(1 + matrix.m22 - matrix.m00 - matrix.m11) * 2);
        w = (matrix.m10 - matrix.m01) / z4;
        x = (matrix.m02 + matrix.m20) / z4;
        y = (matrix.m12 + matrix.m21) / z4;
        z = z4 / 4;
    }

    dest->x = x;
    dest->y = y;
    dest->z = z;
    dest->w = w;

#endif
}