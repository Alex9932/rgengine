#include "soundsource.h"
#include "engine.h"
#include "soundsystem.h"

namespace Engine {

	/////////////////////////////////////////////////////
	// Sound source component
	/////////////////////////////////////////////////////
	SoundSource::SoundSource() : Component(Component_SOUNDSOURCE) {
		alGenSources(1, &m_source);
	}

	SoundSource::~SoundSource() { }

	void SoundSource::Destroy() {
		alDeleteSources(1, &m_source);
		Engine::GetSoundSystem()->DeleteSoundSource(this);
	}

	void SoundSource::Update(Float64 dt) {
		if (m_ent != NULL) {
			SetPosition(m_ent->GetTransform()->GetPosition());
		}
		if (m_buffer != NULL) {
			m_buffer->Update();
		}
	}

	void SoundSource::SetPosition(const vec3& pos) {
		alSource3f(m_source, AL_POSITION, pos.x, pos.y, pos.z);
	}

	void SoundSource::SetBuffer(ISoundBuffer* buffer) {
		if (m_buffer != NULL) {
			m_buffer->SetSource(NULL);
		}
		m_buffer = buffer;
		if (buffer != NULL) {
			buffer->SetSource(this);
		}
	}

	void SoundSource::Play() {
		if (RG_CHECK_FLAG(m_flags, RG_SOUND_PLAYING)) { return; }
		if (m_buffer == NULL) { return; }
		m_buffer->Play();
		alSourcePlay(m_source);
		RG_SET_FLAG(m_flags, RG_SOUND_PLAYING);
	}

	void SoundSource::Stop() {
		if (m_buffer == NULL) { return; }
		RG_RESET_FLAG(m_flags, RG_SOUND_PLAYING);
		alSourceStop(m_source);
		m_buffer->Stop();
		//alSourceUnqueueBuffers();
	}

	void SoundSource::Pause() {
		if (m_buffer == NULL) { return; }
		RG_RESET_FLAG(m_flags, RG_SOUND_PLAYING);
		m_buffer->Pause();
		alSourcePause(m_source);
	}

	void SoundSource::SetRepeat(Bool repeat) {
		if (repeat) {
			RG_SET_FLAG(m_flags, RG_SOUND_LOOPING);
		}
		else {
			RG_RESET_FLAG(m_flags, RG_SOUND_LOOPING);
		}
	}

	Bool SoundSource::IsPaused() {
		if (m_buffer == NULL) { return false; }
		return RG_CHECK_FLAG(m_flags, RG_SOUND_PAUSED);
	}

	Bool SoundSource::IsEnded() {
		if (m_buffer == NULL) { return true; }
		return RG_CHECK_FLAG(m_flags, RG_SOUND_ENDED);
	}

	Bool SoundSource::IsLooping() {
		if (m_buffer == NULL) { return false; }
		return RG_CHECK_FLAG(m_flags, RG_SOUND_LOOPING);
	}

}