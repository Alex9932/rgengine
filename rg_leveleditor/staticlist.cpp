#include "staticlist.h"

#include <world.h>
#include <engine.h>

#include <staticobject.h>

using namespace Engine;

StaticList::StaticList() : UIComponent("Static list") {

}

StaticList::~StaticList() {
}

void StaticList::Draw() {

	World* world = GetWorld();

	if (ImGui::TreeNode("Objects")) {

		Uint32 len = world->GetStaticCount();
		for (Uint32 i = 0; i < len; i++) {
			StaticObject* obj = world->GetStaticObject(i);
			if (ImGui::TreeNode("Obj")) {



				ImGui::TreePop();
			}

		}

		ImGui::TreePop();
	}

}