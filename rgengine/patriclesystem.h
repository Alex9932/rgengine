#ifndef _PARTICLESYSTEM_H
#define _PARTICLESYSTEM_H

#include "entity.h"
#include "rgvector.h"
#include "allocator.h"
#include "engine.h"

namespace Engine {

	typedef struct Particle {
		vec3    pos;
		Float32 lifetime;
		vec3    vel;
		Float32 mul; // (>1 - Increase velocity, <1 - decrease velocity)
	} Particle;

	typedef void (*PFN_PARTICLESPAWN)(Particle*);
	typedef void (*PFN_PARTICLEDELETE)(Particle*);

	typedef struct ParticleEmitterInfo {
		PFN_PARTICLESPAWN  spawn_cb;
		PFN_PARTICLEDELETE delete_cb;    // Not required (may be NULL)
		Float32            lifetime;
		Uint32             max_particles;
	} ParticleEmitterInfo;

	class ParticleEmitter : public Component {

		public:
			ParticleEmitter(ParticleEmitterInfo* info) : Component(Component_PARTICLEEMITTER) {
				
				if (info->spawn_cb == NULL) {
					RG_ERROR_MSG("ParticleEmitter: Invalid arguments!");
				}

				m_particles    = 0;
				m_maxparticles = info->max_particles;
				m_lifetime     = info->lifetime;
				m_cb_spawn     = info->spawn_cb;
				m_cb_delete    = info->delete_cb;

				m_allocator = RG_NEW(PoolAllocator)("ParticleEmitter", info->max_particles, sizeof(Particle));
			}

			~ParticleEmitter() {
				m_allocator->DeallocateAll();
				RG_DELETE(PoolAllocator, m_allocator);
			}

			virtual void Destroy();

			void Update(Float64 dt);

			RG_DECLSPEC void EmitParticle();

		private:
			Uint32             m_particles;
			Uint32             m_maxparticles;
			Float32            m_lifetime;
			PFN_PARTICLESPAWN  m_cb_spawn;
			PFN_PARTICLEDELETE m_cb_delete;
			PoolAllocator*     m_allocator;
	};

	class ParticleSystem {
		public:
			ParticleSystem();
			~ParticleSystem();

			RG_DECLSPEC void UpdateComponents(vec3* viewPos);

			RG_DECLSPEC ParticleEmitter* NewEmitter(ParticleEmitterInfo* info);
			RG_DECLSPEC void DeleteEmitter(ParticleEmitter* comp);

		private:
			PoolAllocator*                m_alloc;
			std::vector<ParticleEmitter*> m_emitters;

	};

}

#endif