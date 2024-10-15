#include "entitylist.h"

#include "popup.h"
//#include "viewport.h"

#include <world.h>
#include <entity.h>
#include <engine.h>

//#include <imgui/ImGuizmo.h>
#include <imgui/ImGuiFileDialog.h>

#include <rgmatrix.h>

using namespace Engine;

EntityList::EntityList() : UIComponent("Entity list") {}
EntityList::~EntityList() {}

void EntityList::Draw() {
	World* world = GetWorld();

	Bool    addEntity = false;
	Entity* toRemove = NULL;

	if (ImGui::TreeNode("Entities")) {
		Uint32 len = world->GetEntityCount();
		for (Uint32 i = 0; i < len; i++) {
			Entity* ent = world->GetEntity(i);
			Transform* transform = ent->GetTransform();
			mat4 model = *transform->GetMatrix();
			char ent_name[128];
			TagComponent* tag = ent->GetComponent(Component_TAG)->AsTagComponent();
			if (tag) {
				SDL_snprintf(ent_name, 128, "%s", tag->GetString());
			}
			else {
				SDL_snprintf(ent_name, 128, "%lx", ent->GetID());
			}
			if (ImGui::TreeNode(ent_name)) {
				ImGui::InputFloat3("Position", transform->GetPosition().array);
				ImGui::InputFloat3("Rotation", transform->GetRotation().array);

				if (ImGui::Button("Rename")) {
					PopupShowInput("Rename", "Enter new entity name");
				}

#if 0
// Manipulation

				if (viewport->IsManipulationResult()) {
					ManipulateResult result = {};
					viewport->GetManipulateResult(&result);
					//rgLogInfo(RG_LOG_SYSTEM, "Matipulation: %f %f %f", result.pos.x, result.pos.y, result.pos.z);
#if 1
					transform->SetPosition(result.pos);
					//transform->SetScale(result.scale);
					transform->Recalculate();
#endif
				}
#endif

				if (ImGui::TreeNode("Components")) {
					//TagComponent* tag = ent->GetComponent(Component_TAG)->AsTagComponent();
					//ImGui::InputText("Name", tag->GetCharBuffer(), tag->GetBufferSize());

					if (ImGui::Button("Attach model")) {
						//ImGuiFileDialog::Instance()->OpenDialog("Open model", "Choose File", ".obj,.pm2,.pmd", ".");
						ImGuiFileDialog::Instance()->OpenDialog("Open model", "Choose File", ".pm2", ".", 1, ent);

					}
					ImGui::TreePop();
				}

				if (ImGui::Button("Remove")) {
					toRemove = ent;
				}

#if 0
				if (!toRemove) {
					viewport->Manipulate(&model, ImGuizmo::TRANSLATE, ImGuizmo::WORLD);
				}
#endif

				ImGui::TreePop();
			}
		}

		if (ImGui::Button("New entity")) {
			addEntity = true;
		}

		ImGui::TreePop();
	}

	if (toRemove) {
		world->FreeEntity(toRemove);
	}

	if (addEntity) {
		Entity* ent = world->NewEntity();
		ent->GetTransform()->SetPosition({ 0, 0, 0 });
		ent->GetTransform()->SetRotation({ 0, 0, 0 });
		ent->GetTransform()->SetScale({ 1, 1, 1 });
	}
}