#include "lightlist.h"

#include <world.h>
#include <engine.h>

#include <lightsystem.h>

#include "viewport.h"

using namespace Engine;

LightList::LightList(Viewport* vp) : UIComponent("Light list") {
	m_vp = vp;
}

LightList::~LightList() {
}

void LightList::Draw() {

	World* world = GetWorld();
	LightSource* toRemove = NULL;
	char src_name[128];

	if (ImGui::TreeNode("Light sources")) {

		Uint32 len = world->GetLightCount();
		for (Uint32 i = 0; i < len; i++) {
			LightSource* src = world->GetLightSource(i);
			SDL_snprintf(src_name, 128, "%x", src->uuid);

			if (ImGui::TreeNode(src_name)) {

				ImGui::InputFloat3("Position", src->source.position.array);

				ImGui::SliderFloat("Intensity", &src->source.intensity, 0, 100, "%.3f", ImGuiSliderFlags_Logarithmic);
				ImGui::ColorPicker3("Color", src->source.color.array);

				if (ImGui::RadioButton("Select", m_vp->GetGizmoID() == src->uuid)) {
					if (m_vp->GetGizmoID() == src->uuid) {
						m_vp->SetGizmoID(0);
					}
					else {
						m_vp->SetGizmoID(src->uuid);
					}
				}

				if (ImGui::Button("Remove")) {
					toRemove = src;
				}

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::Button("New pointlight")) {
		LightSource* src = world->NewLightSource();
		src->source.type = RG_POINTLIGHT;
	}

	if (toRemove) {
		world->FreeLightSource(toRemove);
	}
}