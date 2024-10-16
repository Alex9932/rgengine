#include "entitylist.h"

#include "popup.h"
//#include "viewport.h"

#include <world.h>
#include <entity.h>
#include <engine.h>

//#include <imgui/ImGuizmo.h>
//#include <imgui/ImGuiFileDialog.h>

#include <nfd.h>
#include <nfd_sdl2.h>
#include <window.h>

#include <rgmatrix.h>

#include <render.h>
#include <modelsystem.h>
#include <pm2importer.h>

using namespace Engine;

static PM2Importer pm2Importer;

static void GetNativeWindowHandle(nfdwindowhandle_t* handle) {
	SDL_Window* hwnd = GetWindow();
	NFD_GetNativeWindowFromSDLWindow(hwnd, handle);
}

static Bool ShowOpenDialog(char* dst_path, size_t maxlen) {

	rgLogInfo(RG_LOG_GAME, "Show OpenDialog");
	nfdopendialogu8args_t args = {};
	GetNativeWindowHandle(&args.parentWindow);
	nfdu8filteritem_t filters[2] = { {"PM2 Model file", "pm2"}, {"Wavefront model", "obj"} };
	args.filterList = filters;
	args.filterCount = 2;
	//args.defaultPath = "/";

	nfdu8char_t* outPath;
	nfdresult_t res = NFD_OpenDialogU8_With(&outPath, &args);

	switch (res) {
		case NFD_OKAY: {

			rgLogInfo(RG_LOG_GAME, "File selected: %s", outPath);
			SDL_snprintf(dst_path, maxlen, "%s", outPath);
			NFD_FreePathU8(outPath);
			return true;

		}

		case NFD_CANCEL: { rgLogInfo(RG_LOG_GAME, "NFD: Cancel"); break; }
		case NFD_ERROR: { rgLogInfo(RG_LOG_GAME, "NFD: Internal error"); break; }
		default: { rgLogInfo(RG_LOG_GAME, "NFD: Default case"); break; }
	}

	return false;

}

EntityList::EntityList() : UIComponent("Entity list") {
	if (NFD_Init() != NFD_OKAY) {
		rgLogError(RG_LOG_SYSTEM, "NFD_Init failed: %s\n", NFD_GetError());
		RG_ERROR_MSG("NFD_Init failed! Check log for more information.");
	}
}

EntityList::~EntityList() {
	NFD_Quit();
}


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
						//ImGuiFileDialog::Instance()->OpenDialog("Open model", "Choose File", ".pm2", ".", 1, ent);

						char path[256];
						if(ShowOpenDialog(path, 256)) {
							// Load selected model

							rgLogInfo(RG_LOG_SYSTEM, "Model: %s", path);

							R3DStaticModelInfo objinfo = {};
							pm2Importer.ImportModel(path, &objinfo);
							R3D_StaticModel* mdl_handle1 = Render::R3D_CreateStaticModel(&objinfo);
							pm2Importer.FreeModelData(&objinfo);

							ent->AttachComponent(Render::GetModelSystem()->NewModelComponent(mdl_handle1));
							ent->SetAABB(&objinfo.aabb);

						}

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