#include "entitylist.h"

#include <world.h>
#include <entity.h>
#include <engine.h>

//#include <imgui/ImGuizmo.h>

#include <nfd.h>
#include <nfd_sdl2.h>
#include <window.h>

#include <rgmatrix.h>

#include <render.h>
#include <modelsystem.h>

#include <objimporter.h>
#include <pm2importer.h>
#include <mmdimporter.h>

#include <rgstring.h>

using namespace Engine;

static ObjImporter objImporter;
static PM2Importer pm2Importer;
static PMDImporter pmdImporter;
static PMXImporter pmxImporter;

static void GetNativeWindowHandle(nfdwindowhandle_t* handle) {
	SDL_Window* hwnd = GetWindow();
	NFD_GetNativeWindowFromSDLWindow(hwnd, handle);
}

static Bool ShowOpenDialog(char* dst_path, size_t maxlen) {

	rgLogInfo(RG_LOG_GAME, "Show OpenDialog");
	nfdopendialogu8args_t args = {};
	GetNativeWindowHandle(&args.parentWindow);
	nfdu8filteritem_t filters[4] = { {"PM2 Model file", "pm2"}, {"Wavefront model", "obj"}, {"MMD Polygon model", "pmd"}, {"MMD Extended polygon model", "pmx"} };
	args.filterList = filters;
	args.filterCount = 4;
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

static void ImportPM2(String path, Entity* ent) {
	R3DStaticModelInfo info = {};
	pm2Importer.ImportModel(path, &info);
	R3D_StaticModel* hmdl = Render::R3D_CreateStaticModel(&info);
	pm2Importer.FreeModelData(&info);
	ent->AttachComponent(Render::GetModelSystem()->NewModelComponent(hmdl));
	ent->SetAABB(&info.aabb);
}

static void ImportOBJ(String path, Entity* ent) {
	R3DStaticModelInfo info = {};
	objImporter.ImportModel(path, &info);
	R3D_StaticModel* hmdl = Render::R3D_CreateStaticModel(&info);
	objImporter.FreeModelData(&info);
	ent->AttachComponent(Render::GetModelSystem()->NewModelComponent(hmdl));
	ent->SetAABB(&info.aabb);
}

static void ImportPMD(String path, Entity* ent) {
	R3DRiggedModelInfo info = {};
	pmdImporter.ImportRiggedModel(path, &info);
	R3D_RiggedModel* hmdl = Render::R3D_CreateRiggedModel(&info);
	pmdImporter.FreeRiggedModelData(&info);
	KinematicsModel* kmdl = pmdImporter.ImportKinematicsModel(path);
	ent->AttachComponent(Render::GetModelSystem()->NewRiggedModelComponent(hmdl, kmdl));
	ent->SetAABB(&info.aabb);
}

static void ImportPMX(String path, Entity* ent) {
	R3DRiggedModelInfo info = {};
	pmxImporter.ImportRiggedModel(path, &info);
	R3D_RiggedModel* hmdl = Render::R3D_CreateRiggedModel(&info);
	pmxImporter.FreeRiggedModelData(&info);
	KinematicsModel* kmdl = pmxImporter.ImportKinematicsModel(path);
	ent->AttachComponent(Render::GetModelSystem()->NewRiggedModelComponent(hmdl, kmdl));
	ent->SetAABB(&info.aabb);
}

static void OpenModel(String _path, Entity* ent) {
	char path[256];
	SDL_memset(path, 0, 256);
	size_t j = 0;
	Bool firstseparator = false;
	size_t len = SDL_strlen(_path);
	for (size_t i = 0; i < len; i++) {
		char c = _path[i];
		if (c == '\\') {
			if (!firstseparator) {
				firstseparator = true;
				path[j] = '/';
				j++;
			}
		} else {
			firstseparator = false;
			path[j] = c;
			j++;
		}
	}

	rgLogInfo(RG_LOG_SYSTEM, "Model: %s", path);
	if (rg_strenw(path, "pm2")) {
		ImportPM2(path, ent);
	} else if (rg_strenw(path, "obj")) {
		ImportOBJ(path, ent);
	} else if (rg_strenw(path, "pmd")) {
		ImportPMD(path, ent);
	} else if (rg_strenw(path, "pmx")) {
		ImportPMX(path, ent);
	}
}

EntityList::EntityList() : UIComponent("Entity list") {
	popupidx = PopupNextID();
	rgLogInfo(RG_LOG_SYSTEM, "Popup: Registered ID %d\n", popupidx);

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
			if (tag) { SDL_snprintf(ent_name, 128, "%s", tag->GetString()); }
			else { SDL_snprintf(ent_name, 128, "%x", (Uint32)ent->GetID()); }

			if (ImGui::TreeNode(ent_name)) {
				ImGui::InputFloat3("Position", transform->GetPosition().array);
				ImGui::InputFloat3("Rotation", transform->GetRotation().array);

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
					Bool hasModel = ent->GetComponent(Component_MODELCOMPONENT) != NULL || ent->GetComponent(Component_RIGGEDMODELCOMPONENT) != NULL;
					if (!hasModel) {
						if (ImGui::Button("Attach model")) {
							char path[256];
							if (ShowOpenDialog(path, 256)) {
								// Load selected model
								OpenModel(path, ent);
							}
						}
					} else {
						//ImGui::Text("Model component");
						if (ImGui::TreeNode("Model component")) {
							if (ImGui::Button("Remove")) {
								
							}
							ImGui::TreePop();
						}
					}
					ImGui::TreePop();
				}

				if (ImGui::Button("Rename")) {
					PopupShowInput(popupidx, "Rename", "Enter new entity name");
				}

				ImGui::SameLine();

				if (ImGui::Button("Remove")) {
					toRemove = ent;
				}

#if 0
				if (!toRemove) {
					viewport->Manipulate(&model, ImGuizmo::TRANSLATE, ImGuizmo::WORLD);
				}
#endif

				// Handle popup window
				if (PopupClosed() == popupidx && PopupGetBtnPressed() == POPUP_BTNID_OK) {
					tag->SetString(PopupGetInputBuffer());
					PopupFree();
				}

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
		char entname[64];
		Entity* ent = world->NewEntity();
		ent->GetTransform()->SetPosition({ 0, 0, 0 });
		ent->GetTransform()->SetRotation({ 0, 0, 0 });
		ent->GetTransform()->SetScale({ 1, 1, 1 });

		SDL_snprintf(entname, 64, "Entity {%x}", (Uint32)ent->GetID());
		ent->GetComponent(Component_TAG)->AsTagComponent()->SetString(entname);
	}

}