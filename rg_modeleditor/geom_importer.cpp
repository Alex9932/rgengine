#include "geom_importer.h"

#include <allocator.h>
#include <rgstring.h>
#include "assimputil.h"

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <meshtool.h>

using namespace Engine;

static Assimp::Importer importer;

struct Node {
	String name;
	mat4   local;
	mat4   global;
	Uint32 numChilds;
	Node* childs[128];
	Node* parent;
	aiNode* ainode;
};

struct LoadSkeletonState {
	const aiScene* scene;
	Node* nodes;
	BoneInfo* bones;
	Uint32 bone_counter = 0;
	Uint32 node_idx;
};


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
	const aiScene* scene;
	R3DRiggedModelInfo* info;
	ModelExtraData* extra;
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
	rg_free(info->extra->custom0);

	ImportState* state = (ImportState*)info->userdata;
	RG_DELETE(ImportState, state);

	importer.FreeScene();
	//SDL_memset(info, 0, sizeof(R3DRiggedModelInfo)); // Clear info
}

static const aiScene* LoadScene(String path, String file, char* m_errorstr) {
	char fullpath[512];
	SDL_snprintf(fullpath, 512, "%s/%s", path, file);

	const aiScene* scene = importer.ReadFile(fullpath,
		aiProcess_PopulateArmatureData |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_JoinIdenticalVertices |
		//aiProcess_LimitBoneWeights |
		//aiProcess_PreTransformVertices |
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		//aiProcess_ForceGenNormals |
		//aiProcess_GenSmoothNormals |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_ValidateDataStructure
	);

	//scene->

	if (!scene) {
		String error = importer.GetErrorString();
		SDL_snprintf(m_errorstr, 1024, "ASSIMP ERROR: %s", error);
		rgLogError(RG_LOG_SYSTEM, "%s", m_errorstr);
		return NULL;
	}

	if (!scene->mRootNode) {
		SDL_snprintf(m_errorstr, 1024, "ASSIMP ERROR: No root node!");
		rgLogError(RG_LOG_SYSTEM, "%s", m_errorstr);
		return NULL;
	}

	if (RG_CHECK_FLAG(scene->mFlags, AI_SCENE_FLAGS_INCOMPLETE)) {
		SDL_snprintf(m_errorstr, 1024, "ASSIMP ERROR: Scene incomplete");
		rgLogError(RG_LOG_SYSTEM, "%s", m_errorstr);
		return NULL;
	}

	rgLogInfo(RG_LOG_SYSTEM, "Model loaded: %s", fullpath);

	return scene;
}

static Sint16 FindBone(ImportState* state, size_t bnamehash) {
	if(state->bones.count(bnamehash) == 0) { return -1; }
	return state->bones[bnamehash].index;
}

static Sint32 ChooseBoneSlot(Vertex* vt, Float32 weight) {
	Float32 threshold = 0.01f;//FLT_EPSILON;

	// If weight is too small, ignore it
	if (weight <= threshold) { return -1; }

	// Try to find empty or the smallest weight slot

	Sint32 smallest = 0;
	for (Uint32 i = 0; i < 4; i++) {
		if (vt->w.idx.array[i] == -1) { return i; }

		if (vt->w.weight.array[i] < vt->w.weight.array[smallest]) {
			smallest = i;
		}
	}

	// Check if the new weight is bigger than the smallest one
	if (vt->w.weight.array[smallest] < weight) {
		return smallest;
	}

	// No empty slot and weight is too small
	return -1;
}

static void LoadBoneWeights(ImportState* state, const aiMesh* mesh, Uint32 vertexoffset) {
	for (Uint32 boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		

		aiString name = mesh->mBones[boneIndex]->mName;
		size_t hash = rgHash(name.C_Str(), name.length);

		Sint16 bidx = FindBone(state, hash);
		if (bidx == -1) {
			BoneData bdata = {};
			bdata.index = state->boneCounter;
			bdata.parent = 0; // TODO: Add parent bone id or remove it form structure
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

		//vec4 _weights = {};

		for (Uint32 i = 0; i < numWeights; i++) {
			Uint32 realvidx = weights[i].mVertexId + vertexoffset;
			Float32 weight = weights[i].mWeight;

			Vertex* v = &state->vertices[realvidx];
			Sint32 slot = ChooseBoneSlot(v, weight);

			if (slot != -1) {
				v->w.idx.array[slot] = bidx;
				v->w.weight.array[slot] = weight;
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

		//mat4 t;
		//mat4 t_inv;
		//CopyMatrix(&t, transform);
		//mat4_inverse(&t_inv, t);

		//vec4 v_pos;
		//CopyVector(&v_pos.xyz, mesh->mVertices[i]);
		//v_pos.w = 1;
		//vt.v.pos = (t_inv * v_pos).xyz;
		//vt.v.pos = (t * v_pos).xyz;

		//t_vtx = transform * mesh->mVertices[i];
		t_vtx = mesh->mVertices[i];
		vt.v.pos.x = t_vtx.x;
		vt.v.pos.y = t_vtx.y;
		vt.v.pos.z = t_vtx.z;
		if (mesh->HasNormals()) {
			//t_nrm = normalmat * mesh->mNormals[i];
			t_nrm = mesh->mNormals[i];
			t_nrm.Normalize();
			vt.v.norm.x = t_nrm.x;
			vt.v.norm.y = t_nrm.y;
			vt.v.norm.z = t_nrm.z;
		}
		if (mesh->HasTangentsAndBitangents()) {
			//t_tan = normalmat * mesh->mTangents[i];
			t_tan = mesh->mTangents[i];
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


	for (Uint32 i = 0; i < mesh->mNumVertices; ++i) {
		Uint32 realvidx = i + startvtx;
		Vertex* v = &state->vertices[realvidx];

		// Normalize weights
		Float32 total = v->w.weight.x + v->w.weight.y + v->w.weight.z + v->w.weight.w;
		if (total > 0) {
			v->w.weight.x /= total;
			v->w.weight.y /= total;
			v->w.weight.z /= total;
			v->w.weight.w /= total;
		}
	}
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

	const aiScene* scene = LoadScene(importinfo->path, importinfo->file, m_errorstr);

	ImportState* state = RG_NEW(ImportState);
	importinfo->userdata = state;

	state->info  = importinfo->info.as_rigged;
	state->extra = importinfo->extra;
	state->scene = scene;

	if (!state->scene) {
		rgLogError(RG_LOG_SYSTEM, "ImportModelInfo->scene MUST be a valid pointer to aiScene");
		rgLogError(RG_LOG_SYSTEM, "Failed to load scene!");
		return;
		//rgLogWarn(RG_LOG_SYSTEM, "OK, try to load scene...");
		//state->scene = LoadScene(importinfo->path, importinfo->file);
		//if (!state->scene) {
		//	rgLogError(RG_LOG_SYSTEM, "Failed to load scene!");
		//	return;
		//}
	}


	size_t meshinfoSize = state->scene->mNumMeshes * sizeof(R3D_MatMeshInfo);
	importinfo->info.as_rigged->mInfo = (R3D_MatMeshInfo*)rg_malloc(meshinfoSize);
	importinfo->info.as_rigged->mCount = state->scene->mNumMeshes;

	importinfo->extra->mesh_names = (NameField*)rg_malloc(sizeof(NameField) * state->scene->mNumMeshes);
	importinfo->extra->bone_names = (NameField*)rg_malloc(sizeof(NameField) * 1024); // 1024 bones max
	// Use as normal map file names
	importinfo->extra->custom0    = (NameField*)rg_malloc(sizeof(NameField) * state->scene->mNumMaterials);

	ProcessNode(state, state->scene->mRootNode);


	// De-duplicate vertices

	std::vector<Vertex> new_vertices;
	std::vector<Uint32> new_indices;
	std::map<size_t, Uint32> vtx_hashtable;

	for (Uint32 i = 0; i < state->indices.size(); i++) {
		Uint32 idx = state->indices[i];
		Vertex vtx = state->vertices[idx];
		size_t vtx_hash = rgHash(&vtx, sizeof(R3D_Vertex));

		if (vtx_hashtable.count(vtx_hash) == 0) {
			vtx_hashtable[vtx_hash] = new_vertices.size();
			new_vertices.push_back(vtx);
		}

		new_indices.push_back(vtx_hashtable[vtx_hash]);
	}

	// Calculate required space
	Uint32 totalVertexCount = new_vertices.size();
	Uint32 totalIndexCount  = new_indices.size();
	Uint32 indexSize = 2; // Uint16 by default

	if (totalIndexCount > 0xFFFF) {
		// Use extended size (Uint32)
		indexSize = 4;
	}

	// Allocate memory

	size_t vtxSize = totalVertexCount * sizeof(R3D_Vertex);
	size_t wtxSize = totalVertexCount * sizeof(R3D_Weight);
	size_t idxSize = totalIndexCount  * indexSize;

	importinfo->info.as_rigged->vertices = (R3D_Vertex*)rg_malloc(vtxSize);
	importinfo->info.as_rigged->weights  = (R3D_Weight*)rg_malloc(wtxSize);
	importinfo->info.as_rigged->indices  = rg_malloc(idxSize);
	importinfo->info.as_rigged->vCount   = totalVertexCount;
	importinfo->info.as_rigged->iCount   = totalIndexCount;
	importinfo->info.as_rigged->iType    = (IndexType)indexSize;

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

	// Recalculate normals
	NormalCalculateInfo nrminfo = {};
	nrminfo.idx_count = new_vertices.size();
	nrminfo.vertices  = importinfo->info.as_rigged->vertices;
	nrminfo.indices   = importinfo->info.as_rigged->indices;
	nrminfo.idxtype   = indexSize;
//	RecalculateNormals(&nrminfo);

	// Copy material data
	//info->matInfo

	R3D_MaterialInfo* mtl_array = (R3D_MaterialInfo*)rg_malloc(state->scene->mNumMaterials * sizeof(R3D_MaterialInfo));

	NameField* matnames_all = (NameField*)rg_malloc(sizeof(NameField) * state->scene->mNumMaterials);
	NameField* tex_normals  = (NameField*)rg_malloc(sizeof(NameField) * state->scene->mNumMaterials);
	std::vector<R3D_MaterialInfo*> mtls;
	std::map<size_t, Uint32> mtl_hashtable;

	// Skip default material
	Uint32 matidx = 0;
	if (importinfo->skipFirstMat) {
		matidx = 1;
	}
	
	for (; matidx < state->scene->mNumMaterials; ++matidx) {
		aiMaterial* material = state->scene->mMaterials[matidx];

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
		SDL_memset(tex_normals[matidx].name, 0, 128);
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
			rgLogInfo(RG_LOG_SYSTEM, "Diffuse texture: %s\n", texPath.C_Str());
			SDL_snprintf(minfo->texture, 128, "%s/%s", importinfo->path, texPath.C_Str());
		}

		if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
			rgLogInfo(RG_LOG_SYSTEM, "Normal map: %s\n", texPath.C_Str());
			SDL_snprintf(tex_normals[matidx].name, 128, "%s/%s", importinfo->path, texPath.C_Str());
		}
		else if (material->GetTexture(aiTextureType_HEIGHT, 0, &texPath) == AI_SUCCESS) {
			rgLogInfo(RG_LOG_SYSTEM, "Normal map (height map, old exporter?): %s\n", texPath.C_Str());
			SDL_snprintf(tex_normals[matidx].name, 128, "%s/%s", importinfo->path, texPath.C_Str());
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

	for (Uint32 i = 0; i < state->scene->mNumMaterials; i++) {
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
		SDL_snprintf(importinfo->extra->custom0[actual_idx].name, 128, "%s", tex_normals[midx].name);
	}


	size_t totalMemory = meshinfoSize + matinfoSize + vtxSize + wtxSize + idxSize;
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Used memory: %ldb", totalMemory);
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Vertex buffer: vtx:%ldb idx:%ldb", vtxSize + wtxSize, idxSize);

	rg_free(tex_normals);
	rg_free(mtl_array);
	rg_free(matnames_all);

}


static Uint32 BuildHierarchy(LoadSkeletonState* info, mat4& transform, aiNode* node) {
	mat4 local;
	CopyMatrix(&local, node->mTransformation);
	mat4 global = transform * local;

	Node* n = &info->nodes[info->node_idx];

	n->name      = node->mName.C_Str();
	n->local     = local;
	n->global    = global;
	n->numChilds = node->mNumChildren;
	n->ainode    = node;
	n->parent    = NULL;

	Uint32 processedidx = info->node_idx;
	info->node_idx++;

	for (Uint32 i = 0; i < node->mNumChildren; i++) {
		Uint32 idx = BuildHierarchy(info, global, node->mChildren[i]);
		n->childs[i] = &info->nodes[idx];
		n->childs[i]->parent = n;
	}

	return processedidx;
}

static BoneData* FindBone(ImportState* state, String name) {

	size_t hash = rgHash(name, SDL_strlen(name));
	if (state->bones.count(hash) != 0) {
		return &state->bones[hash];
	}

	rgLogError(RG_LOG_SYSTEM, "Bone %s not found in model", name);

	return NULL;
}

static void BuildSkeleton(ImportState* is, LoadSkeletonState* state, Node* node, Sint32 parent) {

	BoneData* b = FindBone(is, node->name); // Find bone by this name

	Sint32 pidx = parent;

	if (b != NULL) {
		Sint32 bidx = state->bone_counter;
		BoneInfo* dstinfo = &state->bones[b->index];

		SDL_snprintf(dstinfo->name, 32, "%s", node->name);
		CopyMatrix(&dstinfo->offset, b->offset);
		dstinfo->parent = parent;

		dstinfo->has_limit = false;
		dstinfo->limitation = {};

		// Some matrix magic //
		mat4 gtransform;
		mat4_inverse(&gtransform, dstinfo->offset);

		mat4 pinv = MAT4_IDENTITY();
		if (parent != -1) {
			pinv = state->bones[parent].offset;
		}
		mat4_decompose(&dstinfo->offset_pos, &dstinfo->offset_rot, NULL, node->local);

		pidx = bidx;
		rgLogInfo(RG_LOG_SYSTEM, "Bone: %s -> %d", dstinfo->name, dstinfo->parent);
		state->bone_counter++;
	}
	else {
		/* No bone for this node */
	}

	for (Uint32 i = 0; i < node->numChilds; i++) {
		BuildSkeleton(is, state, node->childs[i], pidx);
	}
}

Bool IsIdentity(const aiMatrix4x4& mat) {
	if (mat.a1 == 1 && mat.a2 == 0 && mat.a3 == 0 && mat.a4 == 0 &&
		mat.b1 == 0 && mat.b2 == 1 && mat.b3 == 0 && mat.b4 == 0 &&
		mat.c1 == 0 && mat.c2 == 0 && mat.c3 == 1 && mat.c4 == 0 &&
		mat.d1 == 0 && mat.d2 == 0 && mat.d3 == 0 && mat.d4 == 1) {
		return true;
	}
	return false;
}

KinematicsModel* GeomImporter::LoadSkeleton(ImportModelInfo* info) {
	LoadSkeletonState state = {};

	ImportState* is = (ImportState*)info->userdata;
	if (!is->scene) { return NULL; }

	state.bones = (BoneInfo*)rg_malloc(sizeof(BoneInfo) * 1024);

	state.scene = is->scene;

	state.nodes = (Node*)rg_malloc(sizeof(Node) * is->nodetransforms.size());
	state.node_idx = 0;

	// Build hierarchy
	mat4 root = MAT4_IDENTITY();
	BuildHierarchy(&state, root, state.scene->mRootNode);

	// Build skeleton

	state.bone_counter = 0;
	BuildSkeleton(is, &state, &state.nodes[0], -1);

	rg_free(state.nodes);


	KinematicsModelCreateInfo mk_info = {};

	mk_info.bone_count = state.bone_counter;
	mk_info.bones_info = state.bones;

	mk_info.ik_count = 0;
	mk_info.ik_info = NULL;
	mk_info.buffer_handle = NULL;

	mat4 globalTransform = MAT4_IDENTITY();

#if 0
	// Find root transform
	aiNode* n = state.scene->mRootNode;
	//while (n != NULL) {
		for (Uint32 i = 0; i < n->mNumChildren; i++) {
			aiNode* c = n->mChildren[i];
			if (c->mNumMeshes != 0 || IsIdentity(c->mTransformation)) {

				CopyMatrix(&globalTransform, c->mTransformation);
				goto brk;
			}
			n = c;
		}
		
	//}
	brk:
#endif

	mat4_inverse(&mk_info.globalInv, globalTransform);
	KinematicsModel* km = RG_NEW(KinematicsModel)(&mk_info);

	rg_free(state.bones);

	return km;
}

const aiScene* GeomImporter::GetAIScene(ImportModelInfo* info) {
	return ((ImportState*)info->userdata)->scene;
}