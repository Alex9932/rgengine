#include "transform.h"
#include "rgmatrix.h"

namespace Engine {

	void Transform::Recalculate() {

		if (m_disabled) { return; }

		mat4 local_transform;
		mat4_model(&local_transform, this->m_lPosition, this->m_lRotation, this->m_scale);

//		if (this->m_parent) {
//			mat4* parent = this->m_parent->GetMatrix();
//			this->m_matrix = *parent * local_transform;
//		} else {
			this->m_matrix = local_transform;
//		}

//		mat4_decompose(&this->m_wPosition, &this->m_wRotation, NULL, this->m_matrix);

	}

	void Transform::SetMatrix(mat4* m) {
		vec3 scale;
		vec3 pos;
		quat rot;

		SDL_memcpy(&m_matrix, m, sizeof(mat4));

		mat4_decompose(&pos, &rot, &scale, *m);
		vec3 r = rot.toEuler();
		m_lRotation = r;
		m_lPosition = pos;
		Recalculate();
	}

/*
	vec3& Transform::GetRotation() {
		vec3 e = m_lRotation.toEuler();
		return e;
	}

	void Transform::SetRotation(const vec3& rot) {
		
		Float32 c1 = SDL_cosf(rot.y / 2);
		Float32 s1 = SDL_sinf(rot.y / 2);
		Float32 c2 = SDL_cosf(rot.x / 2);
		Float32 s2 = SDL_sinf(rot.x / 2);
		Float32 c3 = SDL_cosf(rot.z / 2);
		Float32 s3 = SDL_sinf(rot.z / 2);

		Float32 c1c2 = c1 * c2;
		Float32 s1s2 = s1 * s2;

		m_lRotation.w = c1c2 * c3 - s1s2 * s3;
		m_lRotation.x = c1c2 * s3 + s1s2 * c3;
		m_lRotation.y = s1 * c2 * c3 + c1 * s2 * s3;
		m_lRotation.z = c1 * s2 * c3 - s1 * c2 * s3;

	}
*/
}