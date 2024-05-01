#ifndef _PARTICLESYSTEM_H
#define _PARTICLESYSTEM_H

#include "entity.h"
#include "rgvector.h"
#include "allocator.h"
#include "engine.h"
#include "rendertypes.h"

namespace Engine {

	class ParticleEmitter;

	typedef void (*PFN_PARTICLESPAWN)(Particle*, ParticleEmitter*);
	typedef void (*PFN_PARTICLEDELETE)(Particle*, ParticleEmitter*);

	typedef struct ParticleEmitterInfo {
		PFN_PARTICLESPAWN  spawn_cb;
		PFN_PARTICLEDELETE delete_cb;    // Not required (may be NULL)
		Float32            lifetime;
		Uint32             max_particles;
		// Render
		String             sprite_atlas;
		Uint32             width;
		Uint32             height;
	} ParticleEmitterInfo;

	class ParticleEmitter : public Component {

		public:
			ParticleEmitter(ParticleEmitterInfo* info);

			~ParticleEmitter() {
				m_allocator->DeallocateAll();
				RG_DELETE(PoolAllocator, m_allocator);
			}

			virtual void Destroy();

			void Update(Float64 dt);

			RG_DECLSPEC void EmitParticle();

			RG_INLINE Uint32 GetMaxParticles() { return m_maxparticles; }

			RG_INLINE R3D_AtlasHandle*    GetAtlasHandle()  { return m_handle; }
			RG_INLINE R3D_ParticleBuffer* GetBufferHandle() { return m_buffer; }

			RG_INLINE void* GetRawParticleBuffer() { return m_allocator->GetBasePointer(); }

			RG_INLINE Uint32  GetAtlasWidth()  { return m_atlas_width;  }
			RG_INLINE Uint32  GetAtlasHeight() { return m_atlas_height; }
			RG_INLINE Float32 GetMaxTime()     { return m_lifetime;     }

		private:
			Uint32              m_particles;
			Uint32              m_maxparticles;
			Uint32              m_atlas_width;
			Uint32              m_atlas_height;
			Float32             m_lifetime;
			PFN_PARTICLESPAWN   m_cb_spawn;
			PFN_PARTICLEDELETE  m_cb_delete;
			PoolAllocator*      m_allocator;
			R3D_AtlasHandle*    m_handle;
			R3D_ParticleBuffer* m_buffer;
	};

	class ParticleSystem {
		public:
			ParticleSystem();
			~ParticleSystem();

			RG_DECLSPEC void UpdateComponents(vec3* viewPos);

			RG_DECLSPEC ParticleEmitter* NewEmitter(ParticleEmitterInfo* info);
			RG_DECLSPEC void DeleteEmitter(ParticleEmitter* comp);

			RG_DECLSPEC Uint32 GetEmitterCount();
			RG_DECLSPEC ParticleEmitter* GetEmitter(Uint32 id);

		private:
			PoolAllocator*                m_alloc;
			std::vector<ParticleEmitter*> m_emitters;

	};

}

#endif