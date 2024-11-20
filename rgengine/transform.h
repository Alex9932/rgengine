#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "rgmath.h"

namespace Engine {

	class Transform {
		public:
			Transform()  {}
			~Transform() {}

			RG_DECLSPEC void Recalculate();
			
			//RG_INLINE void SetParent(Transform* t) { this->m_parent = t; }
			//RG_INLINE Transform* GetParent() { return this->m_parent; }
			RG_INLINE mat4* GetMatrix() { return &this->m_matrix; }

			//RG_INLINE void SetMatrix(mat4* m) { SDL_memcpy(&m_matrix, m, sizeof(mat4)); }
			RG_DECLSPEC void SetMatrix(mat4* m);

			RG_INLINE vec3& GetPosition() { return this->m_lPosition; }
			RG_INLINE vec3& GetRotation() { return this->m_lRotation; }
			RG_INLINE vec3& GetScale() { return this->m_scale; }
			RG_INLINE void SetPosition(const vec3& pos) { this->m_lPosition = pos; }
			RG_INLINE void SetRotation(const vec3& rot) { this->m_lRotation = rot; }
			RG_INLINE void SetScale(const vec3& scale) { this->m_scale = scale; }

			// Return local transform
			RG_INLINE vec3& GetWorldPosition() { return this->m_lPosition; }
			RG_INLINE vec3& GetWorldRotation() { return this->m_lRotation; }

			//RG_INLINE quat& GetRotationQ() { return this->m_lRotation; }
			//RG_DECLSPEC vec3& GetRotation();
			//RG_INLINE void SetRotationQ(const quat& rot) { this->m_lRotation = rot; }
			//RG_DECLSPEC void SetRotation(const vec3& rot);
			//RG_INLINE vec3& GetWorldPosition() { return this->m_wPosition; }
			//RG_INLINE quat& GetWorldRotation() { return this->m_wRotation; }

			RG_INLINE Bool IsDisabled() { return m_disabled; }

			RG_INLINE void Enable() { m_disabled = false; }
			RG_INLINE void Disable() { m_disabled = true; }


		private:
			mat4       m_matrix    = {};
			vec3	   m_scale     = { 1, 1, 1 };

			// Local position & rotation
			vec3	   m_lPosition = { 0, 0, 0 };
			vec3	   m_lRotation = { 0, 0, 0 };

			// World position & rotation
			//vec3       m_wPosition = { 0, 0, 0 };
			//quat       m_wRotation = { 0, 0, 0, 1 };

			//Transform* m_parent    = NULL;
			Bool       m_disabled  = false;

	};

}

#endif