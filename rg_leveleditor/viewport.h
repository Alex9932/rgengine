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
} ManipulateResult;

class Viewport : public UIComponent {
	public:
		Viewport(Engine::Camera* camera);
		~Viewport();

		virtual void Draw();
		virtual void OnResize(ivec2 newsize);

		void SetImGuizmoRect();

		RG_INLINE void Manipulate(mat4* model, ImGuizmo::OPERATION op, ImGuizmo::MODE mode) {
			m_model = model;
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
			}
		}

		RG_INLINE Bool IsManipulationResult() { return m_isResult; }

	private:
		Engine::Camera*     m_camera     = NULL;
		Bool                m_manipulate = false;
		Bool                m_isResult   = false;
		ImGuizmo::OPERATION m_op         = ImGuizmo::UNIVERSAL;
		ImGuizmo::MODE      m_mode       = ImGuizmo::LOCAL;
		mat4*               m_model      = NULL;
		vec3                m_pos        = {};
		quat                m_rot        = {};
		vec3                m_scale      = {};


};

#endif