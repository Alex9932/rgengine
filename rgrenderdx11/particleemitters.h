#pragma once

// TODO: REWRITE THIS

#include "rgtypes.h"
#include "rendertypes.h";

typedef struct PARTICLE_PSBuffer {
    Uint32  atlas_w;
    Uint32  atlas_h;
    Float32 maxtime;
    Uint32  _offset1;
} PARTICLE_PSBuffer;

Uint32 GetEmitterCount();
Uint32 GetEmitterMaxParticle(Uint32 id);

R3D_AtlasHandle* GetEmitterAtlasHandle(Uint32 id);
R3D_ParticleBuffer* GetEmitterBufferHandle(Uint32 id);

void GetEmitterInfo(Uint32 id, PARTICLE_PSBuffer* buffer);
