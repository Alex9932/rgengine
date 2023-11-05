#include "transform.h"
#include "rgmatrix.h"

namespace Engine {

	void Transform::Recalculate() {

		mat4 local_transform;
		mat4_model(&local_transform, this->m_lPosition, this->m_lRotation, this->m_scale);

		if (this->m_parent) {
			mat4* parent = this->m_parent->GetMatrix();
			this->m_matrix = *parent * local_transform;
		} else {
			this->m_matrix = local_transform;
		}

		mat4_decompose(&this->m_wPosition, &this->m_wRotation, NULL, this->m_matrix);

	}

}