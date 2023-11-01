#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "rgmath.h"

namespace Engine {

	class Transform {
		public:
			Transform()  {}
			~Transform() {}

			RG_DECLSPEC void Recalculate();
			
			RG_INLINE void SetParent(Transform* t) { this->m_parent = t; }
			RG_INLINE Transform* GetParent() { return this->m_parent; }
			RG_INLINE mat4* GetMatrix() { return &this->m_matrix; }

			RG_INLINE vec3& GetPosition() { return this->m_position; }
			RG_INLINE vec3& GetRotation() { return this->m_rotation; }
			RG_INLINE vec3& GetScale() { return this->m_scale; }
			RG_INLINE void SetPosition(const vec3& pos) { this->m_position = pos; }
			RG_INLINE void SetRotation(const vec3& rot) { this->m_rotation = rot; }
			RG_INLINE void SetScale(const vec3& scale) { this->m_scale = scale; }

		private:
			mat4       m_matrix   = {};
			vec3	   m_position = { 0, 0, 0 };
			vec3	   m_rotation = { 0, 0, 0 };
			vec3	   m_scale    = { 0, 0, 0 };
			Transform* m_parent   = NULL;

	};

}

#endif