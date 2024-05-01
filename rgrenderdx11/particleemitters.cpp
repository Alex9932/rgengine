#include "particleemitters.h"

#include <render.h>
#include <particlesystem.h>

Uint32 GetEmitterCount() {
    Engine::ParticleSystem* system = Engine::Render::GetParticleSystem();
    return system->GetEmitterCount();
}

Uint32 GetEmitterMaxParticle(Uint32 id) {
    Engine::ParticleEmitter* emitter = Engine::Render::GetParticleSystem()->GetEmitter(id);
    return emitter->GetMaxParticles();
}

R3D_AtlasHandle* GetEmitterAtlasHandle(Uint32 id) {
    Engine::ParticleEmitter* emitter = Engine::Render::GetParticleSystem()->GetEmitter(id);
    return emitter->GetAtlasHandle();
}

R3D_ParticleBuffer* GetEmitterBufferHandle(Uint32 id) {
    Engine::ParticleEmitter* emitter = Engine::Render::GetParticleSystem()->GetEmitter(id);
    return emitter->GetBufferHandle();
}

void GetEmitterInfo(Uint32 id, PSBuffer* buffer) {
    Engine::ParticleEmitter* emitter = Engine::Render::GetParticleSystem()->GetEmitter(id);

    buffer->atlas_w = emitter->GetAtlasWidth();
    buffer->atlas_h = emitter->GetAtlasHeight();
}