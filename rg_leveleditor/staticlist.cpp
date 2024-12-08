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
#include "dockerglobal.h"

#include <vector>
#include <map>

#include <rgmath.h>

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

static void RecalculateStaticModels(R3DStaticModelInfo* dst, R3DStaticModelInfo* src, std::vector<vec3>& offsets);
static void FreeStaticModels(R3DStaticModelInfo* mdls, Uint32 count);

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

			// Recalculate (separate objects if needed)
			if (docker_splitgeom) {
				std::vector<vec3> offsets;
				R3DStaticModelInfo* models = (R3DStaticModelInfo*)rg_malloc(sizeof(R3DStaticModelInfo) * model.mCount);

				RecalculateStaticModels(models, &model, offsets);

				mat4 transform = MAT4_IDENTITY();
				for (Uint32 i = 0; i < model.mCount; i++) {
					R3D_StaticModel* hmdl = Render::CreateStaticModel(&models[i]);
					// TODO: re-zero position
					mat4_translate(&transform, offsets[i]);
					world->NewStatic(hmdl, &transform, &model.aabb);
				}
				
				FreeStaticModels(models, model.mCount);

				rg_free(models);

			} else {
				R3D_StaticModel* hmdl = Render::CreateStaticModel(&model);
				pm2Importer.FreeModelData(&model);

				mat4 transform = MAT4_IDENTITY();

				// TODO: Use camera position
				mat4_translate(&transform, { 0, 0, 0 });

				world->NewStatic(hmdl, &transform, &model.aabb);
			}
		}
	}

	if (toRemove) {
		world->FreeStatic(toRemove);
	}

}

template <typename T>
static void ProcessIndex(R3DStaticModelInfo* src_info, Uint32 count, Uint32 offset, std::vector<R3D_Vertex>& vertices, std::vector<T>& indices) {

	// Hash - idx
	std::map<size_t, Uint32> vtx_hashtable;
	
	T* src = (T*)src_info->indices;

	for (Uint32 i = 0; i < count; i++) {
		T idx = src[offset + i];

		R3D_Vertex vtx = src_info->vertices[idx];
		size_t vtx_hash = rgHash(&vtx, sizeof(R3D_Vertex));

		if (vtx_hashtable.count(vtx_hash) == 0) {
			vtx_hashtable[vtx_hash] = vertices.size();
			vertices.push_back(vtx);
		}

		indices.push_back(vtx_hashtable[vtx_hash]);
	}
}

static void RecalculateStaticModels(R3DStaticModelInfo* dst, R3DStaticModelInfo* src, std::vector<vec3>& offsets) {

	std::vector<R3D_Vertex> vertices;
	std::vector<Uint16> indices16;
	std::vector<Uint32> indices32;

	for (Uint32 i = 0; i < src->mCount; i++) {
		R3DStaticModelInfo* info = &dst[i];

		info->matCount = 1;
		info->matInfo  = (R3D_MaterialInfo*)rg_malloc(sizeof(R3D_MaterialInfo)); // allocate
		SDL_memcpy(info->matInfo, &src->matInfo[src->mInfo[i].materialIdx], sizeof(R3D_MaterialInfo));

		info->mCount = 1;
		info->mInfo  = (R3D_MatMeshInfo*)rg_malloc(sizeof(R3D_MatMeshInfo)); // allocate
		info->mInfo->materialIdx = 0;
		info->mInfo->indexOffset = 0;
		info->mInfo->indexCount  = src->mInfo[i].indexCount;

		info->iType   = src->iType;
		info->iCount  = src->mInfo[i].indexCount;
		info->indices = rg_malloc(src->iType * src->mInfo[i].indexCount); // allocate

		if (src->iType == RG_INDEX_U16) {
			ProcessIndex(src, src->mInfo[i].indexCount, src->mInfo[i].indexOffset, vertices, indices16);
			Uint16* dst = (Uint16*)info->indices;
			for (Uint32 j = 0; j < src->mInfo[i].indexCount; j++) {
				dst[j] = indices16[j];
			}
		}
		else if (src->iType == RG_INDEX_U32) {
			ProcessIndex(src, src->mInfo[i].indexCount, src->mInfo[i].indexOffset, vertices, indices32);
			Uint32* dst = (Uint32*)info->indices;
			for (Uint32 j = 0; j < src->mInfo[i].indexCount; j++) {
				dst[j] = indices32[j];
			}
		}

		rgLogInfo(RG_LOG_SYSTEM, "Index buffer sizes: %d %d %d", src->mInfo[i].indexCount, indices16.size(), indices32.size());

		// Just copy
		info->vCount   = vertices.size();
		info->vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * vertices.size()); // allocate

		AABB aabb = { {10000, 10000, 10000}, {-10000, -10000, -10000} };
		vec3 offset = {};

		for (Uint32 j = 0; j < info->vCount; j++) {
			info->vertices[j] = vertices[j];

			vec3* c_pos = &vertices[j].pos;
			if (c_pos->x < aabb.min.x) { aabb.min.x = c_pos->x; }
			if (c_pos->y < aabb.min.y) { aabb.min.y = c_pos->y; }
			if (c_pos->z < aabb.min.z) { aabb.min.z = c_pos->z; }
			if (c_pos->x > aabb.max.x) { aabb.max.x = c_pos->x; }
			if (c_pos->y > aabb.max.y) { aabb.max.y = c_pos->y; }
			if (c_pos->z > aabb.max.z) { aabb.max.z = c_pos->z; }

		}

		// TODO: center of model (not min vertex)
		offset = aabb.min;

		// Recalculate vertices
		for (Uint32 j = 0; j < info->vCount; j++) {
			info->vertices[j].pos = info->vertices[j].pos - offset;
		}


		offsets.push_back(offset);

		// TODO: Calculate AABB
		info->aabb = aabb;

		// Reset buffers
		vertices.clear();
		indices16.clear();
		indices32.clear();

	}
}

static void FreeStaticModels(R3DStaticModelInfo* mdls, Uint32 count) {
	for (Uint32 i = 0; i < count; i++) {
		pm2Importer.FreeModelData(&mdls[i]);
	}
}