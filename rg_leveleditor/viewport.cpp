#include "viewport.h"
#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>

#include <render.h>

Viewport::Viewport(Engine::Camera* camera) : UIComponent("Viewport") {
	m_camera = camera;
}

Viewport::~Viewport() {
}

void Viewport::Draw() {
	//ImGui::Text("Viewport size: %dx%d", m_wndsize.x, m_wndsize.y);
	ImVec2 padding = { 0, 0 };
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);

	RenderInfo info = {};
	Engine::Render::GetInfo(&info);
	ImVec2 size;
	size.x = m_wndsize.x;
	size.y = m_wndsize.y-16;
	ImGui::Image((ImTextureID)info.r3d_renderResult, size);

	ImGui::PopStyleVar();
}

void Viewport::OnResize(ivec2 newsize) {
	rgLogInfo(RG_LOG_SYSTEM, "Viewport size: %dx%d", newsize.x, newsize.y);
	this->m_wndsize = newsize;

	m_camera->SetAspect((Float32)newsize.x / (Float32)newsize.y);
	m_camera->ReaclculateProjection();

}

void Viewport::SetImGuizmoRect() {
	//rgLogInfo(RG_LOG_SYSTEM, "Viewport size: %d %d, %dx%d", m_wndpos.x, m_wndpos.y, m_wndsize.x, m_wndsize.y);
	ImGuizmo::SetRect(m_wndpos.x, m_wndpos.y, m_wndsize.x, m_wndsize.y);
}