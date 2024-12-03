#include "staticlist.h"

#include <world.h>
#include <engine.h>

#include <render.h>
#include <staticobject.h>

#include <objimporter.h>
#include <pm2importer.h>
#include <mmdimporter.h>

#include <rgstring.h>

#include <filedialog.h>

#include "viewport.h"

using namespace Engine;

static ObjImporter objImporter;
static PM2Importer pm2Importer;
static PMDImporter pmdImporter;
static PMXImporter pmxImporter;

static void OpenModel(String _path, R3DStaticModelInfo* info) {
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
		}
		else {
			firstseparator = false;
			path[j] = c;
			j++;
		}
	}

	rgLogInfo(RG_LOG_SYSTEM, "Model: %s", path);
	if (rg_strenw(path, "pm2")) {
		pm2Importer.ImportModel(path, info);
	}
	else if (rg_strenw(path, "obj")) {
		objImporter.ImportModel(path, info);
	}
	else if (rg_strenw(path, "pmd")) {
		pmdImporter.ImportModel(path, info);
	}
	else if (rg_strenw(path, "pmx")) {
		pmxImporter.ImportModel(path, info);
	}
}


StaticList::StaticList(Viewport* vp) : UIComponent("Static list") {
	m_vp = vp;
}

StaticList::~StaticList() {
}

void StaticList::Draw() {

	World* world = GetWorld();
	StaticObject* toRemove = NULL;
	char obj_name[128];

	if (ImGui::TreeNode("Objects")) {

		Uint32 len = world->GetStaticCount();
		for (Uint32 i = 0; i < len; i++) {
			StaticObject* obj = world->GetStaticObject(i);

			SDL_snprintf(obj_name, 128, "%x", obj->GetModelHandle());

			if (ImGui::TreeNode(obj_name)) {

				vec3 pos = {};
				quat rot = {};

				mat4_decompose(&pos, &rot, NULL, *obj->GetMatrix());

				ImGui::InputFloat3("Position", pos.array);
				ImGui::InputFloat4("Rotation", rot.v4.array);

				if (ImGui::RadioButton("Select", m_vp->GetGizmoID() == obj->GetID())) {
					if (m_vp->GetGizmoID() == obj->GetID()) {
						m_vp->SetGizmoID(0);
					}
					else {
						m_vp->SetGizmoID(obj->GetID());
					}
				}

				if (ImGui::Button("Remove")) {
					toRemove = obj;
				}

				ImGui::TreePop();
			}

		}

		ImGui::TreePop();
	}

	if (ImGui::Button("New static")) {
		char path[256];
		FD_Filter filters[4] = { {"PM2 Model file", "pm2"}, {"Wavefront model", "obj"}, {"MMD Polygon model", "pmd"}, {"MMD Extended polygon model", "pmx"} };
		if (ShowOpenDialog(path, 256, filters, 4)) {
			R3DStaticModelInfo model = {};
			OpenModel(path, &model);

			R3D_StaticModel* hmdl = Render::CreateStaticModel(&model);
			pm2Importer.FreeModelData(&model);

			mat4 transform = MAT4_IDENTITY();

			// TODO: Use camera position
			mat4_translate(&transform, {0, 0, 0});

			world->NewStatic(hmdl, &transform, &model.aabb);
		}
	}

	if (toRemove) {
		world->FreeStatic(toRemove);
	}

}