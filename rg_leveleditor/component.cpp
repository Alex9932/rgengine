#include "component.h"

#include <imgui/imgui.h>

UIComponent::UIComponent(String name) {
	SDL_snprintf(m_wndname, 64, "%s", name);
}

UIComponent::~UIComponent() {
}

void UIComponent::DrawComponent() {

	ImGui::Begin(m_wndname);

	ImVec2 _wndsize = ImGui::GetWindowSize();
	ivec2 wndsize = { _wndsize.x, _wndsize.y };

	if (this->m_wndsize.x != wndsize.x || this->m_wndsize.y != wndsize.y) {
		OnResize(wndsize);
	}

	this->Draw();

	ImGui::End();
}

void UIComponent::OnResize(ivec2 newsize) {
	this->m_wndsize = newsize;
}