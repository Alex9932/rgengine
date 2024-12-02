#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include "component.h"
#include <camera.h>

typedef struct ManipulateResult {
	vec3 pos;
	Float32 _offset;
	quat rot;
	vec3 scale;
	Float32 _offset2;
	mat4 matrix;
} ManipulateResult;

class Viewport : public UIComponent {
	public:
		Viewport(Engine::Camera* camera);
		~Viewport();

		virtual void Draw();
		virtual void OnResize(ivec2 newsize);

		void SetImGuizmoRect();

		RG_INLINE void Manipulate(mat4* model) {
			m_model = *model;
			m_manipulate = true;
			m_isResult   = false;
		}

		RG_INLINE void GetManipulateResult(ManipulateResult* result) {
			if (m_isResult) {
				result->pos = m_pos;
				result->rot = m_rot;
				result->scale = m_scale;
				result->matrix = m_model;
			}
		}

		RG_INLINE Bool IsManipulationResult() { return m_isResult; }

		RG_INLINE void SetGizmoID(UUID id) { m_gizmoID = id; }
		RG_INLINE UUID GetGizmoID() { return m_gizmoID; }

		RG_INLINE void SetGizmoOp(Uint32 op) { m_guizmo_op = op % 3; }
		RG_INLINE void SetGizmoMode(Uint32 mode) { m_guizmo_mode = mode % 2; }

	private:
		Engine::Camera*     m_camera      = NULL;
		Bool                m_manipulate  = false;
		Bool                m_isResult    = false;
		mat4                m_model       = {};
		vec3                m_pos         = {};
		quat                m_rot         = {};
		vec3                m_scale       = {};

		UUID                m_gizmoID     = 0;

		Uint32              m_guizmo_op   = 0;
		Uint32              m_guizmo_mode = 0;

};

#endif