#define DLL_EXPORT
#include "rgvector.h"

vec3 quat::toEuler() {
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