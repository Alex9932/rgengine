#include "viewport.h"
#include <imgui/imgui.h>

Viewport::Viewport() : UIComponent("Viewport") {
}

Viewport::~Viewport() {
}

void Viewport::Draw() {
	ImGui::Text("Viewport size: %dx%d", m_wndsize.x, m_wndsize.y);
}

void Viewport::OnResize(ivec2 newsize) {
	rgLogInfo(RG_LOG_SYSTEM, "Viewport size: %dx%d", newsize.x, newsize.y);
	this->m_wndsize = newsize;
}