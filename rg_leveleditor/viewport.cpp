#include "viewport.h"
#include <imgui/imgui.h>

#include <render.h>

Viewport::Viewport(Engine::Camera* camera) : UIComponent("Viewport", ImGuiWindowFlags_NoScrollbar) {
	m_camera = camera;
}

Viewport::~Viewport() {
}

void Viewport::Draw() {
	//ImGui::Text("Viewport size: %dx%d", m_wndsize.x, m_wndsize.y);

	RenderInfo info = {};
	Engine::Render::GetInfo(&info);
	ImVec2 size;
	size.x = m_wndsize.x;
	size.y = m_wndsize.y;
	ImGui::Image((ImTextureID)info.r3d_renderResult, size);

	if (m_manipulate) {
		m_manipulate = false;

		ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
		ImGuizmo::SetRect(m_wndpos.x, m_wndpos.y, m_wndsize.x, m_wndsize.y);

		ImGuizmo::Manipulate(m_camera->GetView()->m, m_camera->GetProjection()->m, m_op, m_mode, m_model.m);

		mat4_decompose(&m_pos, &m_rot, &m_scale, m_model);
		m_isResult = true;
	}

}

void Viewport::OnResize(ivec2 newsize) {
	//rgLogInfo(RG_LOG_SYSTEM, "Viewport size: %dx%d", newsize.x, newsize.y);
	this->m_wndsize = newsize;

	m_camera->SetAspect((Float32)newsize.x / (Float32)newsize.y);
	m_camera->ReaclculateProjection();

}

void Viewport::SetImGuizmoRect() {
	//rgLogInfo(RG_LOG_SYSTEM, "Viewport size: %d %d, %dx%d", m_wndpos.x, m_wndpos.y, m_wndsize.x, m_wndsize.y);
	ImGuizmo::SetRect(m_wndpos.x, m_wndpos.y, m_wndsize.x, m_wndsize.y);
}