#include "viewport.h"
#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>

#include <render.h>

static ImGuizmo::OPERATION guizmo_ops[]   = { ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::TRANSLATE | ImGuizmo::ROTATE };
static ImGuizmo::MODE      guizmo_modes[] = { ImGuizmo::LOCAL, ImGuizmo::WORLD };

static String guizmo_ops_string[]   = { "TRANSLATE", "ROTATE", "COMBO" };
static String guizmo_modes_string[] = { "LOCAL", "WORLD" };

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

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	ImVec2 wnd_pos = ImGui::GetWindowPos();
	ImVec2 txt_pos(wnd_pos.x + 10, wnd_pos.y + 10);

	char man_text[128];
	SDL_snprintf(man_text, 128, "%s | %s", guizmo_ops_string[m_guizmo_op], guizmo_modes_string[m_guizmo_mode]);
	draw_list->AddText(txt_pos, ImColor(255.0f, 255.0f, 255.0f, 255.0f), man_text);

	if (m_manipulate) {
		m_manipulate = false;

		ImGuizmo::SetDrawlist(draw_list);
		ImGuizmo::SetRect(m_wndpos.x, m_wndpos.y, m_wndsize.x, m_wndsize.y);

		ImGuizmo::Manipulate(m_camera->GetView()->m, m_camera->GetProjection()->m, guizmo_ops[m_guizmo_op], guizmo_modes[m_guizmo_mode], m_model.m);

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