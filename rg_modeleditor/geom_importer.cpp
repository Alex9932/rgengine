#include "geom_importer.h"

#include <allocator.h>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
//#include <assimp/ProgressHandler.hpp>

#include <map>

struct BoneData {
	Sint16 index;
	Sint16 parent;
	aiMatrix4x4 offset;
};

struct Vertex {
	R3D_Vertex v;
	R3D_Weight w;
};

struct ImportState {
	R3DRiggedModelInfo* info;
	ModelExtraData* extra;
	const aiScene* scene;
	std::map<const aiNode*, aiMatrix4x4> nodetransforms;
	std::map<size_t, BoneData> bones;
	std::vector<Vertex> vertices;
	std::vector<Uint32> indices;
	Uint32 cur_vtx = 0;
	Uint32 cur_idx = 0;
	Uint32 cur_mesh = 0;
	Uint32 boneCounter = 0;
};

void GeomImporter::FreeRiggedModelData(FreeModelInfo* info) {
	rg_free(info->info.as_rigged->vertices);
	rg_free(info->info.as_rigged->weights);
	rg_free(info->info.as_rigged->indices);
	rg_free(info->info.as_rigged->mInfo);
	rg_free(info->info.as_rigged->matInfo);
	rg_free(info->extra->mat_names);
	rg_free(info->extra->mesh_names);
	rg_free(info->extra->bone_names);

	importer.FreeScene();
	//SDL_memset(info, 0, sizeof(R3DRiggedModelInfo)); // Clear info
}

#if 0
class PHandler : public Assimp::ProgressHandler {
	bool Update(float percentage = -1.f) override {
		//rgLogInfo(RG_LOG_SYSTEM, "Loading status: %d %", (Uint32)(percentage * 100));
		return true;
	}

	void UpdateFileRead(int currentStep /*= 0*/, int numberOfSteps /*= 0*/) override {
		float f = numberOfSteps ? currentStep / (float)numberOfSteps : 1.0f;
		rgLogInfo(RG_LOG_SYSTEM, "Read file: %d %", (Uint32)(f * 100));
		Update(f * 0.5f);
	}
	void UpdatePostProcess(int currentStep /*= 0*/, int numberOfSteps /*= 0*/) override {
		float f = numberOfSteps ? currentStep / (float)numberOfSteps : 1.0f;
		rgLogInfo(RG_LOG_SYSTEM, "Postprocess: %d %", (Uint32)(f * 100));
		Update(f * 0.5f + 0.5f);
	}
	void UpdateFileWrite(int currentStep /*= 0*/, int numberOfSteps /*= 0*/) override {
		float f = numberOfSteps ? currentStep / (float)numberOfSteps : 1.0f;
		rgLogInfo(RG_LOG_SYSTEM, "Write file: %d %", (Uint32)(f * 100));
		Update(f * 0.5f);
	}
};

static PHandler handler;

#endif

static Assimp::Importer importer;

static const aiScene* LoadScene(String path, String file) {
	char fullpath[512];
	SDL_snprintf(fullpath, 512, "%s/%s", path, file);

	//importer.SetProgressHandler(&handler);

	const aiScene* scene = importer.ReadFile(fullpath,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		//aiProcess_ForceGenNormals |
		//aiProcess_GenSmoothNormals |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace
	);

	//scene->

	if (!scene) {
		String error = importer.GetErrorString();
		rgLogError(RG_LOG_SYSTEM, "ASSIMP ERROR: %s", error);
		return NULL;
	}

	if (!scene->mRootNode) {
		rgLogError(RG_LOG_SYSTEM, "ASSIMP ERROR: No root node!");
		return NULL;
	}

	if (RG_CHECK_FLAG(scene->mFlags, AI_SCENE_FLAGS_INCOMPLETE)) {
		rgLogError(RG_LOG_SYSTEM, "ASSIMP ERROR: Scene incomplete");
		return NULL;
	}

	rgLogInfo(RG_LOG_SYSTEM, "Model loaded: %s", fullpath);

	return scene;
}

static Sint16 FindBone(ImportState* state, size_t bnamehash) {
	if(state->bones.count(bnamehash) == 0) { return -1; }
	return state->bones[bnamehash].index;
}

static void LoadBoneWeights(ImportState* state, const aiMesh* mesh, Uint32 vertexoffset) {
	for (Uint32 boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		

		aiString name = mesh->mBones[boneIndex]->mName;
		size_t hash = rgHash(name.C_Str(), name.length);

		Sint16 bidx = FindBone(state, hash);
		if (bidx == -1) {
			BoneData bdata = {};
			bdata.index = state->boneCounter;
			bdata.parent = 0; // TODO: Add parent bone id
			bdata.offset = mesh->mBones[boneIndex]->mOffsetMatrix;
			
			state->bones[hash] = bdata;
			bidx = state->boneCounter;
			state->boneCounter++;
		}

		// Save bone name
		char* dstname = state->extra->bone_names[bidx].name;
		SDL_memset(dstname, 0, 128);
		SDL_snprintf(dstname, 128, "%s", name.C_Str());


		aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
		Uint32 numWeights = mesh->mBones[boneIndex]->mNumWeights;

		for (Uint32 i = 0; i < numWeights; i++) {
			Uint32 realvidx = weights[i].mVertexId += vertexoffset;
			Float32 weight = weights[i].mWeight;
			
			for (Uint32 j = 0; j < 4; j++) { // Max 4 bones per vertex
				if (state->vertices[realvidx].w.idx.array[j] < 0) {
					state->vertices[realvidx].w.idx.array[j] = bidx;
					state->vertices[realvidx].w.weight.array[j] = weight;
					break;
				}
			}
		}
	}
}

static void ProcessMesh(ImportState* state, const aiMesh* mesh, const aiMatrix4x4& transform) {

	aiMatrix3x3 normalmat(transform);

	aiVector3D t_vtx;
	aiVector3D t_nrm;
	aiVector3D t_tan;

	Uint32 startvtx = state->cur_vtx;
	for (Uint32 i = 0; i < mesh->mNumVertices; ++i) {
		Vertex vt = {};
		vt.w.idx.x = -1;
		vt.w.idx.y = -1;
		vt.w.idx.z = -1;
		vt.w.idx.w = -1;

		t_vtx = transform * mesh->mVertices[i];
		vt.v.pos.x = t_vtx.x;
		vt.v.pos.y = t_vtx.y;
		vt.v.pos.z = t_vtx.z;
		if (mesh->HasNormals()) {
			t_nrm = normalmat * mesh->mNormals[i];
			t_nrm.Normalize();
			vt.v.norm.x = t_nrm.x;
			vt.v.norm.y = t_nrm.y;
			vt.v.norm.z = t_nrm.z;
		}
		if (mesh->HasTangentsAndBitangents()) {
			t_tan = normalmat * mesh->mTangents[i];
			t_tan.Normalize();
			vt.v.tang.x = t_tan.x;
			vt.v.tang.y = t_tan.y;
			vt.v.tang.z = t_tan.z;
		}
		if (mesh->HasTextureCoords(0)) {
			vt.v.uv.x = mesh->mTextureCoords[0][i].x;
			vt.v.uv.y = mesh->mTextureCoords[0][i].y;
		}

		state->cur_vtx++;

		state->vertices.push_back(vt);
	}

	Uint32 startidx = state->cur_idx;
	for (Uint32 i = 0; i < mesh->mNumFaces; ++i) {
		aiFace* face = &mesh->mFaces[i];
		for (Uint32 j = 0; j < face->mNumIndices; ++j) {
		//for (Sint32 j = face->mNumIndices; j >= 0; --j) {
			state->indices.push_back(face->mIndices[j] + startvtx);
			state->cur_idx++;
		}
	}

	rgLogInfo(RG_LOG_SYSTEM, "[geom] Process mesh: %s", mesh->mName.C_Str());
	SDL_snprintf(state->extra->mesh_names[state->cur_mesh].name, 128, "%s", mesh->mName.C_Str());

	state->info->mInfo[state->cur_mesh].materialIdx = mesh->mMaterialIndex;
	state->info->mInfo[state->cur_mesh].indexOffset = startidx;
	state->info->mInfo[state->cur_mesh].indexCount = state->cur_idx - startidx;
	state->cur_mesh++;

	LoadBoneWeights(state, mesh, startvtx);
}

static void CalculateTransform(ImportState* state, const aiNode* node, aiMatrix4x4* t) {

	if (!node) {
		aiMatrix4x4 identity(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);
		*t = identity;
		rgLogWarn(RG_LOG_SYSTEM, "No transform");
		return;
	}

	if (state->nodetransforms.count(node) != 0) {
		*t = state->nodetransforms[node];
		return;
	}

	aiMatrix4x4 transform = node->mTransformation;
	const aiNode* parent = node->mParent;
	while (parent) {
		transform = parent->mTransformation * transform;
		parent = parent->mParent;
	}

	state->nodetransforms[node] = transform;
	*t = transform;
}

static void ProcessNode(ImportState* state, const aiNode* node) {

	// Calculate transform for this node
	aiMatrix4x4 transform;
	CalculateTransform(state, node, &transform);

	// Process all meshes in this node
	for (Uint32 i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = state->scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(state, mesh, transform);
	}

	// Recursively process children nodes
	for (size_t i = 0; i < node->mNumChildren; i++) {
		ProcessNode(state, node->mChildren[i]);
	}


}

void GeomImporter::ImportRiggedModel(ImportModelInfo* importinfo) {


	ImportState state;
	state.info  = importinfo->info.as_rigged;
	state.extra = importinfo->extra;
	state.scene = (const aiScene*)importinfo->userdata;

	if (!state.scene) {
		rgLogWarn(RG_LOG_SYSTEM, "ImportModelInfo->scene MUST be a valid pointer to aiScene");
		rgLogWarn(RG_LOG_SYSTEM, "OK, try to load scene...");
		state.scene = LoadScene(importinfo->path, importinfo->file);
		if (!state.scene) {
			rgLogError(RG_LOG_SYSTEM, "Failed to load scene!");
			return;
		}
	}


	size_t meshinfoSize = state.scene->mNumMeshes * sizeof(R3D_MatMeshInfo);
	importinfo->info.as_rigged->mInfo = (R3D_MatMeshInfo*)rg_malloc(meshinfoSize);
	importinfo->info.as_rigged->mCount = state.scene->mNumMeshes;

	importinfo->extra->mesh_names = (NameField*)rg_malloc(sizeof(NameField) * state.scene->mNumMeshes);
	importinfo->extra->bone_names = (NameField*)rg_malloc(sizeof(NameField) * 1024); // 1024 bones max

	ProcessNode(&state, state.scene->mRootNode);


	// De-duplicate vertices

	std::vector<Vertex> new_vertices;
	std::vector<Uint32> new_indices;
	std::map<size_t, Uint32> vtx_hashtable;

	for (Uint32 i = 0; i < state.indices.size(); i++) {
		Uint32 idx = state.indices[i];
		Vertex vtx = state.vertices[idx];
		size_t vtx_hash = rgHash(&vtx, sizeof(R3D_Vertex));

		if (vtx_hashtable.count(vtx_hash) == 0) {
			vtx_hashtable[vtx_hash] = new_vertices.size();
			new_vertices.push_back(vtx);
		}

		new_indices.push_back(vtx_hashtable[vtx_hash]);
	}

	// Calculate required space
	Uint32 totalVertexCount = new_vertices.size();
	Uint32 totalIndexCount = new_indices.size();
	Uint32 indexSize = 2; // Uint16 by default

	if (totalIndexCount > 0xFFFF) {
		// Use extended size (Uint32)
		indexSize = 4;
	}

	// Allocate memory

	size_t vtxSize = totalVertexCount * sizeof(R3D_Vertex);
	size_t wtxSize = totalVertexCount * sizeof(R3D_Weight);
	size_t idxSize = totalIndexCount * indexSize;

	importinfo->info.as_rigged->vertices = (R3D_Vertex*)rg_malloc(vtxSize);
	importinfo->info.as_rigged->weights = (R3D_Weight*)rg_malloc(wtxSize);
	importinfo->info.as_rigged->indices = rg_malloc(idxSize);
	importinfo->info.as_rigged->vCount = totalVertexCount;
	importinfo->info.as_rigged->iCount = totalIndexCount;
	importinfo->info.as_rigged->iType = (IndexType)indexSize;

	// Copy de-duplicated vertex data

	for (Uint32 i = 0; i < new_vertices.size(); i++) {
		importinfo->info.as_rigged->vertices[i] = new_vertices[i].v;
		importinfo->info.as_rigged->weights[i]  = new_vertices[i].w;
	}

	if (indexSize == 2) {
		Uint16* idx = (Uint16*)importinfo->info.as_rigged->indices;
		for (Uint32 i = 0; i < new_indices.size(); i++) {
			idx[i] = (Uint16)new_indices[i];
		}

	}
	else {
		Uint32* idx = (Uint32*)importinfo->info.as_rigged->indices;
		for (Uint32 i = 0; i < new_indices.size(); i++) {
			idx[i] = new_indices[i];
		}
	}

	// Copy material data
	//info->matInfo

	R3D_MaterialInfo* mtl_array = (R3D_MaterialInfo*)rg_malloc(state.scene->mNumMaterials * sizeof(R3D_MaterialInfo));

	NameField* matnames_all = (NameField*)rg_malloc(sizeof(NameField) * state.scene->mNumMaterials);
	std::vector<R3D_MaterialInfo*> mtls;
	std::map<size_t, Uint32> mtl_hashtable;

	// Skip default material
	Uint32 matidx = 0;
	if (importinfo->skipFirstMat) {
		matidx = 1;
	}
	
	for (; matidx < state.scene->mNumMaterials; ++matidx) {
		aiMaterial* material = state.scene->mMaterials[matidx];

		//R3D_MaterialInfo* minfo = &info->matInfo[i];
		R3D_MaterialInfo* minfo = &mtl_array[matidx];

#if 1
		// Color
		aiColor3D diffuse(1.0f, 1.0f, 1.0f);
		material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);

		minfo->color.r = diffuse.r;
		minfo->color.g = diffuse.g;
		minfo->color.b = diffuse.b;

		SDL_snprintf(matnames_all[matidx].name, 128, "material_%d", matidx);

		aiString name;
		if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
			rgLogInfo(RG_LOG_SYSTEM, "Material %d: %s\n", matidx, name.C_Str());
			SDL_snprintf(matnames_all[matidx].name, 128, "%s", name.C_Str());
		}

		aiString texPath;
		SDL_memset(minfo->texture, 0, 128);
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
			rgLogInfo(RG_LOG_SYSTEM, "Diffuse texture: %s\n", texPath.C_Str());
			SDL_snprintf(minfo->texture, 128, "%s/%s", importinfo->path, texPath.C_Str());
		}


		size_t mtl_hash = rgHash(minfo, sizeof(R3D_MaterialInfo));

		if (mtl_hashtable.count(mtl_hash) == 0) {
			mtl_hashtable[mtl_hash] = mtls.size();
			mtls.push_back(minfo);
		}

		rgLogInfo(RG_LOG_SYSTEM, "Material [%lx]: %f %f %f %s", mtl_hash, diffuse.r, diffuse.g, diffuse.b, minfo->texture);

#endif
#if 0
		// TMP: Generate random color per material
		Float32 r = (Float32)(rand() % 10000) / 10000.0f;
		Float32 g = (Float32)(rand() % 10000) / 10000.0f;
		Float32 b = (Float32)(rand() % 10000) / 10000.0f;
		minfo->color.r = r;
		minfo->color.g = g;
		minfo->color.b = b;

		rgLogInfo(RG_LOG_SYSTEM, "COLOR: %f %f %f", r, g, b);
#endif
	}



	Uint32 matcount = mtls.size();
	importinfo->info.as_rigged->matCount = matcount;

	size_t matinfoSize = matcount * sizeof(R3D_MaterialInfo);
	importinfo->info.as_rigged->matInfo = (R3D_MaterialInfo*)rg_malloc(matinfoSize);

	importinfo->extra->mat_names = (NameField*)rg_malloc(sizeof(NameField) * matcount);

	for (Uint32 i = 0; i < state.scene->mNumMaterials; i++) {
		size_t mtl_hash = rgHash(&mtl_array[i], sizeof(R3D_MaterialInfo));
		Uint32 actual_idx = mtl_hashtable[mtl_hash];
		SDL_snprintf(importinfo->extra->mat_names[actual_idx].name, 128, "%s", matnames_all[i].name);
	}

	for (Uint32 i = 0; i < matcount; i++) {
		SDL_memcpy(&importinfo->info.as_rigged->matInfo[i], mtls[i], sizeof(R3D_MaterialInfo));
	}

	// and replace material index in meshes

	for (Uint32 i = 0; i < importinfo->info.as_rigged->mCount; i++) {
		Uint32 midx = importinfo->info.as_rigged->mInfo[i].materialIdx;

		size_t mtl_hash = rgHash(&mtl_array[midx], sizeof(R3D_MaterialInfo));
		Uint32 actual_idx = mtl_hashtable[mtl_hash];

		importinfo->info.as_rigged->mInfo[i].materialIdx = actual_idx;
	}


	size_t totalMemory = meshinfoSize + matinfoSize + vtxSize + wtxSize + idxSize;
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Used memory: %ldb", totalMemory);
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Vertex buffer: vtx:%ldb idx:%ldb", vtxSize + wtxSize, idxSize);

	rg_free(mtl_array);
	rg_free(matnames_all);

}
