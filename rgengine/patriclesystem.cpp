#include "patriclesystem.h"
#include "rgthread.h"

#define RG_EMITTERPOOL_SIZE 1024

namespace Engine {

	void ParticleEmitter::Destroy() {

	}

	void ParticleEmitter::Update(Float64 dt) {
		Particle* array_ptr   = (Particle*)m_allocator->GetBasePointer();
		Particle* current_ptr = NULL;

		for (Uint32 i = 0; i < m_maxparticles; i++) {
			current_ptr = &array_ptr[i];

			// Skip "dead" particle
			if (current_ptr->lifetime < 0) { continue; }

			// "Kill" old particle
			if (current_ptr->lifetime == 0) {
				if (m_cb_delete) { m_cb_delete(current_ptr); }
				current_ptr->lifetime = -1; // "Dead" particle
				m_allocator->Deallocate(current_ptr);
				continue;
			}

			current_ptr->pos += current_ptr->vel * (Float32)dt;
			current_ptr->vel *= current_ptr->mul;
			current_ptr->lifetime -= (Float32)dt;

		}
	}

	void ParticleEmitter::EmitParticle() {
		if(m_particles >= m_maxparticles) { return; }

		Particle* part_ptr = (Particle*)m_allocator->Allocate();
		if (part_ptr == NULL) { return; }

		if (m_cb_spawn) {
			m_cb_spawn(part_ptr);
		}
	}


	ParticleSystem::ParticleSystem() {
		m_alloc = RG_NEW_CLASS(GetDefaultAllocator(), PoolAllocator)("Pointlight pool", RG_EMITTERPOOL_SIZE, sizeof(ParticleEmitter));
	}

	ParticleSystem::~ParticleSystem() {
		RG_DELETE_CLASS(GetDefaultAllocator(), PoolAllocator, m_alloc);
	}

	static void EmitterUpdater(void* userdata) {
		ParticleEmitter* emitter = (ParticleEmitter*)userdata;
		emitter->Update(GetDeltaTime());
	}

	void ParticleSystem::UpdateComponents(vec3* viewPos) {
		// TODO
		if (viewPos != NULL) {
			// Sort emitters
		}

		//Dispatch update tasks
		Task t;
		for (size_t i = 0; i < m_emitters.size(); i++) {
			ParticleEmitter* emitter = m_emitters[i];
			t.proc     = EmitterUpdater;
			t.userdata = emitter;
			ThreadDispatch(&t);
		}
		
	}

	ParticleEmitter* ParticleSystem::NewEmitter(ParticleEmitterInfo* info) {
		ParticleEmitter* comp = RG_NEW_CLASS(m_alloc, ParticleEmitter)(info);
		m_emitters.push_back(comp);
		return comp;
	}

	void ParticleSystem::DeleteEmitter(ParticleEmitter* comp) {
		std::vector<ParticleEmitter*>::iterator it = m_emitters.begin();
		for (; it != m_emitters.end(); it++) {
			if (*it == comp) {
				*it = std::move(m_emitters.back());
				m_emitters.pop_back();
				RG_DELETE_CLASS(this->m_alloc, ParticleEmitter, comp);
				break;
			}
		}
	}

}