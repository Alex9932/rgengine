#include "viewport.h"
#include <imgui/imgui.h>

#include <render.h>

Viewport::Viewport(Engine::Camera* camera) : UIComponent("Viewport") {
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
}

void Viewport::OnResize(ivec2 newsize) {
	rgLogInfo(RG_LOG_SYSTEM, "Viewport size: %dx%d", newsize.x, newsize.y);
	this->m_wndsize = newsize;

	m_camera->SetAspect((Float32)newsize.x / (Float32)newsize.y);
	m_camera->ReaclculateProjection();

}