#include "transform.h"
#include "rgmatrix.h"

namespace Engine {

	void Transform::Recalculate() {

		mat4 local_transform;
		mat4_model(&local_transform, this->m_position, this->m_rotation, this->m_scale);

		if (this->m_parent) {
			mat4* parent = this->m_parent->GetMatrix();
			this->m_matrix = *parent * local_transform;
		} else {
			this->m_matrix = local_transform;
		}

	}

}