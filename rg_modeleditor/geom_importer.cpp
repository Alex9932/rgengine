#include "geom_importer.h"

#include <allocator.h>
#include <rgstring.h>
#include "assimputil.h"

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/BaseImporter.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <meshtool.h>
#include <filesystem.h>

#include <cjson/cJSON.h>

#include <vector>
#include <unordered_map>

using namespace Engine;

static Assimp::Importer importer;

#define RG_GEOM_DEBUG_REST 1

#if 0
struct Node {
	String name;
	mat4   local;
	mat4   global;
	//Uint32 numChilds;
	//Node* childs[128];
	std::vector<Node*> childs;
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
#endif

struct BoneData {
	Sint16 index;
	Sint16 parent;
	aiMatrix4x4 offset;
};

struct BoneRec {
	std::string   name;
	aiMatrix4x4   offset;
	const aiNode* node;
	Sint32        parent;
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
	std::map<size_t, BoneData> bones; // name hash -> skinning bone
	std::vector<BoneRec> bone_list;
	std::vector<Vertex> vertices;
	std::vector<Uint32> indices;
	Uint32 cur_vtx = 0;
	Uint32 cur_idx = 0;
	Uint32 cur_mesh = 0;
	Uint32 mesh_capacity = 0;
	Uint32 boneCounter = 0;
	Bool   isVRM = false;
	Bool   animationOnly = false;
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

	//SDL_memset(info, 0, sizeof(R3DRiggedModelInfo)); // Clear info
}

struct GLBHeader {
	Uint32 magic;
	Uint32 version;
	Uint32 length;
};

struct GLBChunkHeader {
	Uint32 chunkLength;
	Uint32 chunkType;
};

#if 0
static void ParseJSONVRM(String jsonstr, BoneInfo* bones) {
	cJSON* root = cJSON_Parse(jsonstr);
	if (!root) { rgLogError(RG_LOG_SYSTEM, "Invalid JSON chunk!"); return; }
	cJSON* extensions = cJSON_GetObjectItem(root, "extensions");
	if (!extensions) { rgLogError(RG_LOG_SYSTEM, "No extensions found!"); return; }
	cJSON* VRM = cJSON_GetObjectItem(extensions, "VRM");
	if (!VRM) { rgLogError(RG_LOG_SYSTEM, "No VRM extension!"); return; }

	cJSON* humanoid = cJSON_GetObjectItem(VRM, "humanoid");
	if (!humanoid) { rgLogError(RG_LOG_SYSTEM, "Invalid VRM extension!"); return; }
	cJSON* humanBones = cJSON_GetObjectItem(humanoid, "humanBones"); // Array
	if (!humanBones) { rgLogError(RG_LOG_SYSTEM, "No \"humanBones\"!"); return; }

	for (Sint32 i = 0; i < cJSON_GetArraySize(humanBones); i++) {
		cJSON* humanBone = cJSON_GetArrayItem(humanBones, i); // Bone object
		cJSON* obj_bone_name = cJSON_GetObjectItem(humanBone, "bone");
		cJSON* obj_attach_to = cJSON_GetObjectItem(humanBone, "node");
		cJSON* obj_use_defaults = cJSON_GetObjectItem(humanBone, "useDefaultValues");
		String bone_name = cJSON_GetStringValue(obj_bone_name);
		Sint32 attach_to = (Sint32)cJSON_GetNumberValue(obj_attach_to);
		
		SDL_snprintf(bones[attach_to].name, 32, "%s", bone_name);
	}

	//cJSON_Print(humanBones);
	cJSON_Delete(root);
}

static void ProcessVRM(String path, BoneInfo* bones) {

	FSReader reader(path);
	//reader.IsStreamAvailable();

	//FSMemoryInputStream stream(res->data, res->length);
	GLBHeader hdr = {};
	reader.Read(&hdr, sizeof(GLBHeader));

	void* jsonptr = NULL;
	GLBChunkHeader chunkhdr = {};
	while (!reader.EndOfStream()) {
		reader.Read(&chunkhdr, sizeof(GLBChunkHeader));
		rgLogInfo(RG_LOG_SYSTEM, "GLB Chunk: Type: 0x%08X, Length: %d", chunkhdr.chunkType, chunkhdr.chunkLength);
		chunkhdr.chunkLength;

		if (chunkhdr.chunkType == 0x4E4F534A) {
			jsonptr = rg_malloc(chunkhdr.chunkLength + 1);
			SDL_memset(jsonptr, 0, chunkhdr.chunkLength + 1);
			reader.Read(jsonptr, chunkhdr.chunkLength);
			String jsonstr = (String)jsonptr;

			ParseJSONVRM(jsonstr, bones);

			//rgLogInfo(RG_LOG_SYSTEM, "JSON:\n%s", jsonstr);

			//FSWriter writer("readed.json");
			//writer.Write(jsonptr, chunkhdr.chunkLength);
			//writer.Flush();
			rg_free(jsonptr);
			break;
		}
		else {
			reader.Seek(chunkhdr.chunkLength, RG_FS_SEEK_CUR);
		}
	}


}
#endif

struct ImportSceneData {
	Assimp::Importer importer;
};

void FreeScene(ImportSceneData* ptr) {
	if (!ptr) { return; }
	ptr->importer.FreeScene();
	//delete ptr;
	ptr->~ImportSceneData();
	rg_free(ptr);
}

static const aiScene* LoadScene(ImportState* state, String path, String file, char* m_errorstr, size_t msglen, ImportSceneData** data) {
	char fullpath[512];
	SDL_snprintf(fullpath, 512, "%s/%s", path, file);

	Assimp::Importer* cimporter = &importer;

	if (data) {
		*data = new(rg_malloc(sizeof(ImportSceneData))) ImportSceneData();
		cimporter = &(*data)->importer;
	}

	const aiScene* scene;

	Uint32 flags = aiProcess_ValidateDataStructure;
	if (!state->animationOnly) {
		flags |=
			//aiProcess_ConvertToLeftHanded |
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
			aiProcess_CalcTangentSpace;
	}

#if 0
	if (rg_strenw(file, "vrm")) {
		// VRM files are actually GLTF (GLB) files with extra data
		state->isVRM = true;
		Resource* res = GetResource(fullpath);
		scene = cimporter->ReadFileFromMemory(res->data, res->length, flags, "fake.glb");

		FreeResource(res);

	} else {
		scene = cimporter->ReadFile(fullpath, flags);
	}
#else
	scene = cimporter->ReadFile(fullpath, flags);
#endif


	if (!scene) {
		String error = cimporter->GetErrorString();
		SDL_snprintf(m_errorstr, msglen, "ASSIMP ERROR: %s", error);
		rgLogError(RG_LOG_SYSTEM, "%s", m_errorstr);
		if (data) { FreeScene(*data); *data = NULL; }
		return scene;
	}

	if (!scene->mRootNode) {
		SDL_snprintf(m_errorstr, msglen, "ASSIMP ERROR: No root node!");
		rgLogError(RG_LOG_SYSTEM, "%s", m_errorstr);
		if (data) { FreeScene(*data); *data = NULL; }
		return scene;
	}

	if (RG_CHECK_FLAG(scene->mFlags, AI_SCENE_FLAGS_INCOMPLETE) && !state->animationOnly) {
		SDL_snprintf(m_errorstr, msglen, "ASSIMP ERROR: Scene incomplete");
		rgLogError(RG_LOG_SYSTEM, "%s", m_errorstr);
		if (data) { FreeScene(*data); *data = NULL; }
		return scene;
	}

	rgLogInfo(RG_LOG_SYSTEM, "Model loaded: %s", fullpath);

	// Extract textures
	char texpath[128];
	for (Uint32 i = 0; i < scene->mNumTextures; i++) {
		aiTexture* tex = scene->mTextures[i];
		//SDL_snprintf(texpath, 128, "%s/*%s", path, &tex->mFilename.C_Str()[1]);
		if (tex->mHeight != 0) {
			rgLogWarn(RG_LOG_SYSTEM, "SKIP RAW TEXTURE: %s", tex->mFilename.C_Str());
			continue;
		}
		SDL_snprintf(texpath, 128, "%s/%d", path, i);
		rgLogInfo(RG_LOG_SYSTEM, "Copy embedding [%u]: %s", tex->mWidth, texpath);
		FSWriter writer(texpath);
		writer.Write(tex->pcData, tex->mWidth);
		writer.Flush();
	}

	return scene;
}

const aiScene* LoadScene(String path, String file, char* errorstr, size_t msglen, ImportSceneData** data, Bool animOnly) {
	ImportState state = {};
	state.animationOnly = animOnly;
	return LoadScene(&state, path, file, errorstr, msglen, data);
}

static Sint16 FindBoneIndex(ImportState* state, size_t bnamehash) {
	auto it = state->bones.find(bnamehash); // iterator
	if (it == state->bones.end()) { return -1; }
	return it->second.index;
}

struct BoneSource {
	std::string name;
	aiMatrix4x4 offset;
	bool        registered = false;
};

static void CalculateTransform(ImportState* state, const aiNode* node, aiMatrix4x4* t);

static bool MarkSkeletonNodes(const aiNode* node, const std::map<size_t, BoneSource>& sources, std::set<const aiNode*>& needed) {
	bool need = sources.find(rgHash(node->mName.C_Str(), node->mName.length)) != sources.end();
	for (Uint32 i = 0; i < node->mNumChildren; i++) {
		if (MarkSkeletonNodes(node->mChildren[i], sources, needed)) { need = true; } // no short-circuit: visit all children
	}
	if (need) { needed.insert(node); }
	return need;
}

static void RegisterSkeletonNode(ImportState* state, const aiNode* node, Sint32 parent,
	const std::map<size_t, BoneSource>& sources, const std::set<const aiNode*>& needed) {
	if (needed.count(node) == 0) { return; }

	if (state->bone_list.size() >= RG_MAX_BONE_COUNT) {
		rgLogError(RG_LOG_SYSTEM, "[geom] Too many bones (max %d), node '%s' dropped", RG_MAX_BONE_COUNT, node->mName.C_Str());
		return;
	}

	Sint32 idx = (Sint32)state->bone_list.size();
	size_t hash = rgHash(node->mName.C_Str(), node->mName.length);

	BoneRec rec;
	rec.name = node->mName.C_Str();
	rec.node = node;
	rec.parent = parent;

	auto it = sources.find(hash);
	if (it != sources.end() && state->bones.count(hash) == 0) {
		// Skinning bone: vertex weights refer to this index
		rec.offset = it->second.offset;

		BoneData bdata = {};
		bdata.index = (Sint16)idx;
		bdata.parent = (Sint16)parent;
		bdata.offset = rec.offset;
		state->bones[hash] = bdata;
	}
	else {
		// Joint-only node (scene root, armature, helper). No vertex references it, but it is part of the
		// chain. offset = inverse(global bind) makes its skin matrix identity in the rest pose, which is
		// what rigidly bound (unweighted) vertices need.
		aiMatrix4x4 g;
		CalculateTransform(state, node, &g);
		g.Inverse();
		rec.offset = g;
	}

	state->bone_list.push_back(rec);
	state->boneCounter = (Uint32)state->bone_list.size();

	char* dstname = state->extra->bone_names[idx].name;
	SDL_memset(dstname, 0, 128);
	SDL_snprintf(dstname, 128, "%s", node->mName.C_Str());

	for (Uint32 i = 0; i < node->mNumChildren; i++) {
		RegisterSkeletonNode(state, node->mChildren[i], idx, sources, needed);
	}
}

static void RegisterBones(ImportState* state) {
	std::map<size_t, BoneSource> sources;
	const aiScene* scene = state->scene;

	for (Uint32 m = 0; m < scene->mNumMeshes; m++) {
		const aiMesh* mesh = scene->mMeshes[m];
		for (Uint32 b = 0; b < mesh->mNumBones; b++) {
			const aiBone* bone = mesh->mBones[b];
			size_t hash = rgHash(bone->mName.C_Str(), bone->mName.length);
			if (sources.count(hash) == 0) {
				BoneSource src;
				src.name = bone->mName.C_Str();
				src.offset = bone->mOffsetMatrix;
				sources[hash] = src;
			}
		}
	}

	if (sources.empty()) { return; }

	std::set<const aiNode*> needed;
	MarkSkeletonNodes(scene->mRootNode, sources, needed);
	RegisterSkeletonNode(state, scene->mRootNode, -1, sources, needed);

	if (state->bones.size() != sources.size()) {
		rgLogWarn(RG_LOG_SYSTEM, "[geom] %u skinning bones have no node in the hierarchy (their weights are ignored)",
			(Uint32)(sources.size() - state->bones.size()));
	}

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

		Sint16 bidx = FindBoneIndex(state, hash);
		if (bidx == -1) {
			rgLogError(RG_LOG_SYSTEM, "[geom] Bone '%s' was not registered", name.C_Str());
			continue;
		}

		aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
		Uint32 numWeights = mesh->mBones[boneIndex]->mNumWeights;

		for (Uint32 i = 0; i < numWeights; i++) {
			Uint32 realvidx = weights[i].mVertexId + vertexoffset;
			Float32 weight = weights[i].mWeight;

			if (realvidx >= state->vertices.size()) { continue; }

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

	if (state->cur_mesh >= state->mesh_capacity) {
		rgLogError(RG_LOG_SYSTEM, "[geom] Mesh table overflow, mesh '%s' skipped", mesh->mName.C_Str());
		return;
	}

	aiMatrix3x3 normalmat(transform);
	normalmat.Inverse().Transpose();
	aiMatrix3x3 tangentmat(transform);

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
		else {
			// Bind it rigidly to the root bone.
			v->w.idx.x = 0;
			v->w.idx.y = 0;
			v->w.idx.z = 0;
			v->w.idx.w = 0;
			v->w.weight.x = 1.0f;
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

	auto it = state->nodetransforms.find(node);
	if (it != state->nodetransforms.end()) {
		*t = it->second;
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
		aiMatrix4x4 mesh_transform = mesh->HasBones() ? aiMatrix4x4() : transform; // aiMatrix4x4() == identity
		ProcessMesh(state, mesh, mesh_transform);
	}

	// Recursively process children nodes
	for (size_t i = 0; i < node->mNumChildren; i++) {
		ProcessNode(state, node->mChildren[i]);
	}
}

static Uint32 CountMeshRefs(const aiNode* node) {
	Uint32 n = node->mNumMeshes;
	for (Uint32 i = 0; i < node->mNumChildren; i++) {
		n += CountMeshRefs(node->mChildren[i]);
	}
	return n;
}

#if RG_GEOM_DEBUG_REST
static void DebugRestSkinning(ImportState* is, KinematicsModel* km) {
	km->RecalculateTransform();
	mat4* palette = km->GetTransforms();
	Uint32 bc = km->GetBoneCount();

	float rmin[3] = { 1e30f,  1e30f,  1e30f }, rmax[3] = { -1e30f, -1e30f, -1e30f };
	float smin[3] = { 1e30f,  1e30f,  1e30f }, smax[3] = { -1e30f, -1e30f, -1e30f };
	double dsum[3] = { 0, 0, 0 };
	size_t counted = 0, unweighted = 0;
	float  maxd = 0.0f;
	Uint32 maxd_bone = 0;

	for (size_t vi = 0; vi < is->vertices.size(); vi++) {
		const Vertex& v = is->vertices[vi];
		float p[3] = { v.v.pos.x, v.v.pos.y, v.v.pos.z };
		float s[3] = { 0, 0, 0 };
		float wsum = 0.0f;
		Uint32 top_bone = 0;
		float  top_w = -1.0f;

		for (Uint32 k = 0; k < 4; k++) {
			Sint32 bi = v.w.idx.array[k];
			float  w = v.w.weight.array[k];
			if (bi < 0 || bi >= (Sint32)bc || w <= 0.0f) { continue; }
			const mat4& m = palette[bi];
			s[0] += w * (m.m00 * p[0] + m.m01 * p[1] + m.m02 * p[2] + m.m03);
			s[1] += w * (m.m10 * p[0] + m.m11 * p[1] + m.m12 * p[2] + m.m13);
			s[2] += w * (m.m20 * p[0] + m.m21 * p[1] + m.m22 * p[2] + m.m23);
			wsum += w;
			if (w > top_w) { top_w = w; top_bone = (Uint32)bi; }
		}
		if (wsum <= 0.0f) { unweighted++; continue; }

		float d2 = 0.0f;
		for (int c = 0; c < 3; c++) {
			if (p[c] < rmin[c]) rmin[c] = p[c];
			if (p[c] > rmax[c]) rmax[c] = p[c];
			if (s[c] < smin[c]) smin[c] = s[c];
			if (s[c] > smax[c]) smax[c] = s[c];
			dsum[c] += (s[c] - p[c]);
			d2 += (s[c] - p[c]) * (s[c] - p[c]);
		}
		float d = SDL_sqrtf(d2);
		if (d > maxd) { maxd = d; maxd_bone = top_bone; }
		counted++;
	}

	rgLogInfo(RG_LOG_SYSTEM, "[geom][debug] bones: %u, vertices: %u (skinned %u, unweighted %u)\n",
		bc, (Uint32)is->vertices.size(), (Uint32)counted, (Uint32)unweighted);
	rgLogInfo(RG_LOG_SYSTEM, "[geom][debug] RAW  bbox min (%f %f %f) max (%f %f %f)\n",
		rmin[0], rmin[1], rmin[2], rmax[0], rmax[1], rmax[2]);
	rgLogInfo(RG_LOG_SYSTEM, "[geom][debug] SKIN bbox min (%f %f %f) max (%f %f %f)\n",
		smin[0], smin[1], smin[2], smax[0], smax[1], smax[2]);
	if (counted > 0) {
		rgLogInfo(RG_LOG_SYSTEM, "[geom][debug] mean (skinned - raw) = (%f %f %f), max |delta| = %f at a vertex whose main bone is '%s'\n",
			(float)(dsum[0] / counted), (float)(dsum[1] / counted), (float)(dsum[2] / counted),
			maxd, km->GetBone(maxd_bone) ? km->GetBone(maxd_bone)->name : "?");
	}

	// The top of the chain and the biggest bone positions (a few bones very far away = broken transform / scale)
	Uint32 far_bone = 0;
	float  far_dist = 0.0f;
	for (Uint32 i = 0; i < bc; i++) {
		Bone* b = km->GetBone(i);
		if (i < 8) {
			rgLogInfo(RG_LOG_SYSTEM, "[geom][debug] bone %u '%s' parent %d  local pos (%f %f %f) scale (%f %f %f)  global pos (%f %f %f)\n",
				i, b->name, (Sint32)b->parent,
				b->offset_pos.x, b->offset_pos.y, b->offset_pos.z,
				b->offset_scale.x, b->offset_scale.y, b->offset_scale.z,
				b->transform.m03, b->transform.m13, b->transform.m23);
		}
		float dd = SDL_sqrtf(b->transform.m03 * b->transform.m03 + b->transform.m13 * b->transform.m13 + b->transform.m23 * b->transform.m23);
		if (dd > far_dist) { far_dist = dd; far_bone = i; }
	}
	if (bc > 0) {
		Bone* b = km->GetBone(far_bone);
		rgLogInfo(RG_LOG_SYSTEM, "[geom][debug] farthest bone from origin: %u '%s' parent %d, distance %f, local scale (%f %f %f)\n",
			far_bone, b->name, (Sint32)b->parent, far_dist, b->offset_scale.x, b->offset_scale.y, b->offset_scale.z);
	}
}
#endif

void GeomImporter::ImportRiggedModel(ImportModelInfo* importinfo) {

	ImportState* state = RG_NEW(ImportState);
	const aiScene* scene = LoadScene(state, importinfo->path, importinfo->file, m_errorstr, 1024, NULL);

	if (!state->scene) {
		rgLogError(RG_LOG_SYSTEM, "ImportModelInfo->scene MUST be a valid pointer to aiScene");
		rgLogError(RG_LOG_SYSTEM, "Failed to load scene!");
		RG_DELETE(ImportState, state);
		importinfo->userdata = NULL;
		return;
	}

	importinfo->userdata = state;

	state->info  = importinfo->info.as_rigged;
	state->extra = importinfo->extra;
	state->scene = scene;

	Uint32 meshCount = CountMeshRefs(scene->mRootNode);
	Uint32 meshAlloc = meshCount > 0 ? meshCount : 1;
	Uint32 allocMats = scene->mNumMaterials > 0 ? scene->mNumMaterials : 1;

	size_t meshinfoSize = meshAlloc * sizeof(R3D_MatMeshInfo);
	importinfo->info.as_rigged->mInfo = (R3D_MatMeshInfo*)rg_malloc(meshinfoSize);
	SDL_memset(importinfo->info.as_rigged->mInfo, 0, meshinfoSize);
	importinfo->info.as_rigged->mCount = 0;
	state->mesh_capacity = meshAlloc;

	importinfo->extra->mesh_names = (NameField*)rg_malloc(sizeof(NameField) * meshAlloc);
	SDL_memset(importinfo->extra->mesh_names, 0, sizeof(NameField) * meshAlloc);

	importinfo->extra->bone_names = (NameField*)rg_malloc(sizeof(NameField) * RG_MAX_BONE_COUNT);
	SDL_memset(importinfo->extra->bone_names, 0, sizeof(NameField) * RG_MAX_BONE_COUNT);

	// Use as normal map file names
	importinfo->extra->custom0 = (NameField*)rg_malloc(sizeof(NameField) * allocMats);
	SDL_memset(importinfo->extra->custom0, 0, sizeof(NameField) * allocMats);

	importinfo->extra->mat_names = (NameField*)rg_malloc(sizeof(NameField) * allocMats);
	SDL_memset(importinfo->extra->mat_names, 0, sizeof(NameField) * allocMats);

	// Bones first (parent-first order), then meshes/weights
	RegisterBones(state);

	ProcessNode(state, state->scene->mRootNode);
	importinfo->info.as_rigged->mCount = state->cur_mesh;

	// De-duplicate vertices

	std::vector<Vertex> new_vertices;
	std::vector<Uint32> new_indices;
	std::unordered_multimap<size_t, Uint32> vtx_hashtable;
	new_vertices.reserve(state->vertices.size());
	new_indices.reserve(state->indices.size());

	for (Uint32 i = 0; i < state->indices.size(); i++) {
		Uint32 idx = state->indices[i];
		const Vertex& vtx = state->vertices[idx];
		size_t vtx_hash = rgHash(&vtx, sizeof(Vertex));

		Sint64 found = -1;
		auto range = vtx_hashtable.equal_range(vtx_hash);
		for (auto it = range.first; it != range.second; ++it) {
			if (SDL_memcmp(&new_vertices[it->second], &vtx, sizeof(Vertex)) == 0) {
				found = it->second;
				break;
			}
		}

		if (found < 0) {
			found = (Sint64)new_vertices.size();
			vtx_hashtable.insert({ vtx_hash, (Uint32)found });
			new_vertices.push_back(vtx);
		}

		new_indices.push_back((Uint32)found);
	}

	// Calculate required space
	Uint32 totalVertexCount = (Uint32)new_vertices.size();
	Uint32 totalIndexCount  = (Uint32)new_indices.size();
	Uint32 indexSize = 2; // Uint16 by default

	if (totalVertexCount > 0xFFFF) {
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

#if 0
	// Recalculate normals
	NormalCalculateInfo nrminfo = {};
	nrminfo.idx_count = new_vertices.size();
	nrminfo.vertices  = importinfo->info.as_rigged->vertices;
	nrminfo.indices   = importinfo->info.as_rigged->indices;
	nrminfo.idxtype   = indexSize;
	RecalculateNormals(&nrminfo);
#endif

	// Copy material data

	Uint32 numMats = state->scene->mNumMaterials;

	R3D_MaterialInfo* mtl_array = (R3D_MaterialInfo*)rg_malloc(allocMats * sizeof(R3D_MaterialInfo));

	NameField* matnames_all = (NameField*)rg_malloc(sizeof(NameField) * allocMats);
	NameField* tex_normals = (NameField*)rg_malloc(sizeof(NameField) * allocMats);
	SDL_memset(mtl_array, 0, allocMats * sizeof(R3D_MaterialInfo));
	SDL_memset(matnames_all, 0, sizeof(NameField) * allocMats);
	SDL_memset(tex_normals, 0, sizeof(NameField) * allocMats);

	std::vector<R3D_MaterialInfo*> mtls;
	std::vector<size_t> mtl_keys(allocMats, 0);
	std::map<size_t, Uint32> mtl_hashtable;

	// Skip default material
	Uint32 firstMat = importinfo->skipFirstMat ? 1 : 0;
	
	for (Uint32 matidx = firstMat; matidx < numMats; ++matidx) {
		aiMaterial* material = state->scene->mMaterials[matidx];

		R3D_MaterialInfo* minfo = &mtl_array[matidx];

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
			String texstr = texPath.C_Str();
			if (texstr[0] == '*') {
				// Embedded texture
				texstr = &texstr[1];
			}
			rgLogInfo(RG_LOG_SYSTEM, "Diffuse texture: %s\n", texstr);
			SDL_snprintf(minfo->texture, 128, "%s/%s", importinfo->path, texstr);
		}

		if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
			String texstr = texPath.C_Str();
			if (texstr[0] == '*') {
				// Embedded texture
				texstr = &texstr[1];
			}
			rgLogInfo(RG_LOG_SYSTEM, "Normal map: %s\n", texstr);
			SDL_snprintf(tex_normals[matidx].name, 128, "%s/%s", importinfo->path, texstr);
		}
		else if (material->GetTexture(aiTextureType_HEIGHT, 0, &texPath) == AI_SUCCESS) {
			rgLogInfo(RG_LOG_SYSTEM, "Normal map (height map, old exporter?): %s\n", texPath.C_Str());
			SDL_snprintf(tex_normals[matidx].name, 128, "%s/%s", importinfo->path, texPath.C_Str());
		}
		

#if 0
		size_t mtl_hash = rgHash(minfo, sizeof(R3D_MaterialInfo));
#else
		size_t mtl_hash = rgHash(minfo, sizeof(R3D_MaterialInfo));
		size_t nrm_hash = rgHash(tex_normals[matidx].name, SDL_strlen(tex_normals[matidx].name));
		mtl_hash ^= nrm_hash + 0x9e3779b97f4a7c15ull + (mtl_hash << 6) + (mtl_hash >> 2);
		mtl_keys[matidx] = mtl_hash;
#endif

		if (mtl_hashtable.count(mtl_hash) == 0) {
			Uint32 actual_idx = (Uint32)mtls.size();
			mtl_hashtable[mtl_hash] = actual_idx;
			mtls.push_back(minfo);

			SDL_snprintf(importinfo->extra->mat_names[actual_idx].name, 128, "%s", matnames_all[matidx].name);
			SDL_snprintf(importinfo->extra->custom0[actual_idx].name, 128, "%s", tex_normals[matidx].name);
		}

		rgLogInfo(RG_LOG_SYSTEM, "Material [%zx]: %f %f %f %s", mtl_hash, diffuse.r, diffuse.g, diffuse.b, minfo->texture);

	}

	Uint32 matcount = (Uint32)mtls.size();
	importinfo->info.as_rigged->matCount = matcount;

	size_t matinfoSize = (matcount > 0 ? matcount : 1) * sizeof(R3D_MaterialInfo);
	importinfo->info.as_rigged->matInfo = (R3D_MaterialInfo*)rg_malloc(matinfoSize);
	SDL_memset(importinfo->info.as_rigged->matInfo, 0, matinfoSize);

	for (Uint32 i = 0; i < matcount; i++) {
		SDL_memcpy(&importinfo->info.as_rigged->matInfo[i], mtls[i], sizeof(R3D_MaterialInfo));
	}

	// and replace material index in meshes

	for (Uint32 i = 0; i < importinfo->info.as_rigged->mCount; i++) {
		Uint32 midx = importinfo->info.as_rigged->mInfo[i].materialIdx;

		Uint32 actual_idx = 0; // fallback (e.g. skipped default material)
		if (midx >= firstMat && midx < numMats) {
			auto it = mtl_hashtable.find(mtl_keys[midx]);
			if (it != mtl_hashtable.end()) { actual_idx = it->second; }
		}

		importinfo->info.as_rigged->mInfo[i].materialIdx = actual_idx;
	}


	size_t totalMemory = meshinfoSize + matinfoSize + vtxSize + wtxSize + idxSize;
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Used memory: %zub", totalMemory);
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Vertex buffer: vtx:%zub idx:%zub", vtxSize + wtxSize, idxSize);

	rg_free(tex_normals);
	rg_free(mtl_array);
	rg_free(matnames_all);
}

KinematicsModel* GeomImporter::LoadSkeleton(ImportModelInfo* info) {
	ImportState* is = (ImportState*)info->userdata;
	if (!is || !is->scene) { return NULL; }

	Uint32 boneCount = (Uint32)is->bone_list.size();
	BoneInfo* bones = (BoneInfo*)rg_malloc(sizeof(BoneInfo) * RG_MAX_BONE_COUNT);
	SDL_memset(bones, 0, sizeof(BoneInfo) * RG_MAX_BONE_COUNT);

	for (Uint32 i = 0; i < boneCount; i++) {
		const BoneRec& rec = is->bone_list[i];
		BoneInfo* bi = &bones[i];
		SDL_snprintf(bi->name, 32, "%s", rec.name.c_str());
		bi->parent = rec.parent;
		CopyMatrix(&bi->offset, rec.offset);

		aiVector3D   s, p;
		aiQuaternion q;
		rec.node->mTransformation.Decompose(s, q, p);
		CopyVector(&bi->offset_pos, p);
		CopyVector(&bi->offset_rot, q);
		CopyVector(&bi->offset_scale, s);

		bi->has_limit = false;
		bi->limitation = {};
		
		if (bi->parent >= (Sint32)i) {
			rgLogError(RG_LOG_SYSTEM, "[geom] Bone %u (%s) has parent %d >= own index", i, bi->name, (Sint32)bi->parent);
		}
	}

#if 0
	// Process VRM extra data
	if (is->isVRM) {
		char fullpath[512];
		SDL_snprintf(fullpath, 512, "%s/%s", info->path, info->file);
		ProcessVRM(fullpath, state.bones);
	}
#endif

	KinematicsModelCreateInfo mk_info = {};

	mk_info.bone_count = boneCount;
	mk_info.bones_info = bones;

	mk_info.ik_count = 0;
	mk_info.ik_info = NULL;

	mat4 globalTransform = MAT4_IDENTITY();
	mat4_inverse(&mk_info.globalInv, globalTransform);
	KinematicsModel* km = RG_NEW(KinematicsModel)(&mk_info);

	rg_free(bones);

	km->ValidateRestPose();
#if RG_GEOM_DEBUG_REST
	DebugRestSkinning(is, km);
#endif

	return km;
}

const aiScene* GeomImporter::GetAIScene(ImportModelInfo* info) {
	ImportState* is = (ImportState*)info->userdata;
	return is ? is->scene : NULL;
	//return ((ImportState*)info->userdata)->scene;
}