#define DLL_EXPORT
#include "rgvector.h"

quat quat::nlerp(const quat& q, float t) const {
    quat r;

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

    return r.normalize();
}

quat quat::slerp(const quat& q, float t) const {

    quat bq = q;
    float d = x * bq.x + y * bq.y + z * bq.z + w * bq.w;
    float s = 1.0f - t;
    if (d < 0.0f) {
        bq.x = -bq.x;
        bq.y = -bq.y;
        bq.z = -bq.z;
        bq.w = -bq.w;
        d = -d;
    }

    d = SDL_clamp(d, -1.0f, 1.0f);

    if (d > 0.9995f) {
        quat r;
        r.x = x * s + bq.x * t;
        r.y = y * s + bq.y * t;
        r.z = z * s + bq.z * t;
        r.w = w * s + bq.w * t;
        return r;
    }

    float theta = SDL_acosf(d);
    float sintheta = SDL_sinf(theta);

    s = SDL_sinf(s * theta) / sintheta;
    t = SDL_sinf(t * theta) / sintheta;

    quat r;
    r.x = x * s + bq.x * t;
    r.y = y * s + bq.y * t;
    r.z = z * s + bq.z * t;
    r.w = w * s + bq.w * t;
    return r;
}

vec3 quat::toEuler() const {
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