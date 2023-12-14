#include "component.h"

UIComponent::UIComponent(String name, ImU32 flags) {
	SDL_snprintf(m_wndname, 64, "%s", name);
	m_flags = flags;
}

UIComponent::~UIComponent() {
}

void UIComponent::DrawComponent() {

	ImGui::Begin(m_wndname, NULL, m_flags);

	ImVec2 _wndpos  = ImGui::GetWindowPos();
	ivec2 wndpos = { _wndpos.x, _wndpos.y };
	if (this->m_wndpos.x != wndpos.x || this->m_wndpos.y != wndpos.y) {
		OnReposition(wndpos);
	}

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

void UIComponent::OnReposition(ivec2 newpos) {
	this->m_wndpos = newpos;
}