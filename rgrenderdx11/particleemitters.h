#pragma once

// TODO: REWRITE THIS

#include "rgtypes.h"
#include "rendertypes.h";

typedef struct PSBuffer {
    Uint32 atlas_w;
    Uint32 atlas_h;
    Uint32 _offset0;
    Uint32 _offset1;
} PSBuffer;

Uint32 GetEmitterCount();
Uint32 GetEmitterMaxParticle(Uint32 id);

R3D_AtlasHandle* GetEmitterAtlasHandle(Uint32 id);
R3D_ParticleBuffer* GetEmitterBufferHandle(Uint32 id);

void GetEmitterInfo(Uint32 id, PSBuffer* buffer);
