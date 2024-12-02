#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include "component.h"
#include <camera.h>
#include <imgui/ImGuizmo.h>

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

		RG_INLINE void Manipulate(mat4* model, ImGuizmo::OPERATION op, ImGuizmo::MODE mode) {
			m_model = *model;
			m_mode  = mode;
			m_op    = op;
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

	private:
		Engine::Camera*     m_camera     = NULL;
		Bool                m_manipulate = false;
		Bool                m_isResult   = false;
		ImGuizmo::OPERATION m_op         = ImGuizmo::UNIVERSAL;
		ImGuizmo::MODE      m_mode       = ImGuizmo::LOCAL;
		mat4                m_model      = {};
		vec3                m_pos        = {};
		quat                m_rot        = {};
		vec3                m_scale      = {};

		UUID                m_gizmoID    = 0;


};

#endif