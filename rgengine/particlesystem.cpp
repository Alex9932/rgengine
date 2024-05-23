#include "particlesystem.h"
#include "rgthread.h"
#include "render.h"

#define RG_EMITTERPOOL_SIZE 1024
#define RG_PARTICLE_DEAD_LIFETIME -10000

namespace Engine {

	ParticleEmitter::ParticleEmitter(ParticleEmitterInfo* info) : Component(Component_PARTICLEEMITTER) {

		if (info->spawn_cb == NULL) {
			RG_ERROR_MSG("ParticleEmitter: Invalid arguments!");
		}

		m_particles    = 0;
		m_maxparticles = info->max_particles;
		m_lifetime     = info->lifetime;
		m_cb_spawn     = info->spawn_cb;
		m_cb_delete    = info->delete_cb;

		R3DCreateBufferInfo bufferInfo = {};
		bufferInfo.len         = sizeof(Particle) * info->max_particles;
		bufferInfo.initialData = NULL;

		m_atlas_width  = info->width;
		m_atlas_height = info->height;
		m_handle       = Render::CreateAtlas(info->sprite_atlas);
		m_buffer       = Render::CreateParticleBuffer(&bufferInfo);

		m_allocator = RG_NEW(PoolAllocator)("ParticleEmitter", info->max_particles, sizeof(Particle));
	}

	void ParticleEmitter::Destroy() {
		Render::DestroyAtlas(m_handle);
		Render::DestroyParticleBuffer(m_buffer);
	}

	void ParticleEmitter::Update(Float64 dt) {
		Particle* array_ptr   = (Particle*)m_allocator->GetBasePointer();
		Particle* current_ptr = NULL;

		for (Uint32 i = 0; i < m_maxparticles; i++) {
			current_ptr = &array_ptr[i];

			// "Kill" old particle
			if (current_ptr->lifetime < 0 && current_ptr->lifetime > RG_PARTICLE_DEAD_LIFETIME) {
				if (m_cb_delete) { m_cb_delete(current_ptr, this); }
				current_ptr->lifetime = RG_PARTICLE_DEAD_LIFETIME; // "Dead" particle
				m_allocator->Deallocate(current_ptr);
				continue;
			}

			// Skip "dead" particle
			if (current_ptr->lifetime < 0) { continue; }

			current_ptr->pos += current_ptr->vel * (Float32)dt;
			current_ptr->vel *= current_ptr->mul;
			current_ptr->lifetime -= (Float32)dt;
#if 0
			rgLogWarn(RG_LOG_SYSTEM, "Pos: %f %f %f, Vel: %f %f %f",
				current_ptr->pos.x,
				current_ptr->pos.y,
				current_ptr->pos.z,
				current_ptr->vel.x,
				current_ptr->vel.y,
				current_ptr->vel.z);
#endif
		}
	}

	void ParticleEmitter::EmitParticle() {
		if(m_particles >= m_maxparticles) { return; }

		Particle* part_ptr = (Particle*)m_allocator->Allocate();
		if (part_ptr == NULL) { return; }

		rgLogInfo(RG_LOG_GAME, "Emit particle");
		m_cb_spawn(part_ptr, this);
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

		R3DUpdateBufferInfo info = {};
		
		for (size_t i = 0; i < m_emitters.size(); i++) {
			ParticleEmitter* emitter = m_emitters[i];
			info.handle_particle = emitter->GetBufferHandle();
			info.offset = 0;
			info.length = sizeof(Particle) * emitter->GetMaxParticles();
			info.data    = emitter->GetRawParticleBuffer();
			Render::UpdateParticleBuffer(&info);
		}

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

	Uint32 ParticleSystem::GetEmitterCount() {
		return m_emitters.size();
	}

	ParticleEmitter* ParticleSystem::GetEmitter(Uint32 id) {
		return m_emitters[id];
	}

}