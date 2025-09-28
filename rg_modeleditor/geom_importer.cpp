#include "geom_importer.h"

#include <allocator.h>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <map>

static const aiNode* FindMeshNode(aiNode* node, Uint32 idx) {
	for (Uint32 i = 0; i < node->mNumMeshes; ++i) {
		if (node->mMeshes[i] == idx) { return node; }
	}

	for (Uint32 i = 0; i < node->mNumChildren; ++i) {
		const aiNode* cnode = FindMeshNode(node->mChildren[i], idx);
		if (cnode) { return cnode; }
	}

	// NO NODE
	return NULL;
}

static void FindTransforms(std::map<Uint32, const aiNode*>& nmap, const aiScene* scene) {
	for (Uint32 m = 0; m < scene->mNumMeshes; m++) {
		const aiNode* node = FindMeshNode(scene->mRootNode, m);
		nmap[m] = node;
	}
}

static void GetNodeTransform(const aiNode* node, aiMatrix4x4* t) {
	if (!node) {
		aiMatrix4x4 identity(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);
		*t = identity;
		rgLogWarn(RG_LOG_SYSTEM, "No transform" );
		return;
	}
	aiMatrix4x4 transform = node->mTransformation;
	const aiNode* parent = node->mParent;
	while (parent) {
		transform = parent->mTransformation * transform;
		parent = parent->mParent;
	}

	*t = transform;
}

void FreeStaticModel(R3DStaticModelInfo* info, ModelExtraData* extra) {
	if (!info) return;
	rg_free(info->vertices);
	rg_free(info->indices);
	rg_free(info->mInfo);
	rg_free(info->matInfo);
	rg_free(extra->mat_names);
	rg_free(extra->mesh_names);
	SDL_memset(info, 0, sizeof(R3DStaticModelInfo)); // Clear info
}

static Assimp::Importer importer;

static const aiScene* LoadScene(String path, String file) {
	char fullpath[512];
	SDL_snprintf(fullpath, 512, "%s/%s", path, file);

	const aiScene* scene = importer.ReadFile(fullpath,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		//aiProcess_ForceGenNormals |
		//aiProcess_GenSmoothNormals |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace
	);

	if (!scene) {
		String error = importer.GetErrorString();
		rgLogError(RG_LOG_SYSTEM, "ASSIMP ERROR: %s", error);
		return NULL;
	}

	rgLogInfo(RG_LOG_SYSTEM, "Model loaded: %s", fullpath);

	return scene;
}
#if 0
static void LoadBoneWeights(const aiScene* scene, aiMesh* mesh, R3D_Weight* weights) {
	for (Uint32 boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		Sint16 bidx = -1;

		aiString name = mesh->mBones[boneIndex]->mName;

		Sint16 f_bone = FindBone(name.C_Str());
		if (f_bone == -1) {
			
		}
		else {
			bidx = f_bone;
		}
	}
}
#endif
void ImportStaticModel(ImportStaticModelInfo* importinfo) {

	const aiScene* scene = LoadScene(importinfo->path, importinfo->file);
	if (!scene) { return; }


	// Find transforms
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Calculating transformations");
	std::map<Uint32, const aiNode*> nodemap;
	FindTransforms(nodemap, scene);

	aiMatrix4x4* transforms = (aiMatrix4x4*)rg_malloc(scene->mNumMeshes * sizeof(aiMatrix4x4));
	for (Uint32 m = 0; m < scene->mNumMeshes; m++) {
		GetNodeTransform(nodemap[m], &transforms[m]);
	}

	size_t meshinfoSize = scene->mNumMeshes * sizeof(R3D_MatMeshInfo);
	importinfo->info->mInfo = (R3D_MatMeshInfo*)rg_malloc(meshinfoSize);
	importinfo->info->mCount = scene->mNumMeshes;

	importinfo->extra->mesh_names = (NameField*)rg_malloc(sizeof(NameField) * scene->mNumMeshes);

	// Copy vertex data
	std::vector<R3D_Vertex> vertices;
	std::vector<Uint32> indices;
	std::map<size_t, Uint32> vtx_hashtable;

	rgLogInfo(RG_LOG_SYSTEM, "[geom] Copy data");
	Uint32 cur_vtx = 0;
	Uint32 cur_idx = 0;
	for (Uint32 m = 0; m < scene->mNumMeshes; m++) {
		aiMesh* mesh = scene->mMeshes[m];

		rgLogInfo(RG_LOG_SYSTEM, "[geom] Process mesh: %s", mesh->mName.C_Str());
		SDL_snprintf(importinfo->extra->mesh_names[m].name, 128, "%s", mesh->mName.C_Str());

		aiMatrix4x4 transform = transforms[m];
		aiMatrix3x3 normalmat(transform);

		aiVector3D t_vtx;
		aiVector3D t_nrm;
		aiVector3D t_tan;

		Uint32 startvtx = cur_vtx;
		for (Uint32 i = 0; i < mesh->mNumVertices; ++i) {
			R3D_Vertex v = {};

			t_vtx = transform * mesh->mVertices[i];
			v.pos.x = t_vtx.x;
			v.pos.y = t_vtx.y;
			v.pos.z = t_vtx.z;
			if (mesh->HasNormals()) {
				t_nrm = normalmat * mesh->mNormals[i];
				t_nrm.Normalize();
				v.norm.x = t_nrm.x;
				v.norm.y = t_nrm.y;
				v.norm.z = t_nrm.z;
			}
			if (mesh->HasTangentsAndBitangents()) {
				t_tan = normalmat * mesh->mTangents[i];
				t_tan.Normalize();
				v.tang.x = t_tan.x;
				v.tang.y = t_tan.y;
				v.tang.z = t_tan.z;
			}
			if (mesh->HasTextureCoords(0)) {
				v.uv.x = mesh->mTextureCoords[0][i].x;
				v.uv.y = mesh->mTextureCoords[0][i].y;
			}

#if 0
			info->vertices[cur_vtx] = v;
#endif

			cur_vtx++;

			vertices.push_back(v);
		}

		Uint32 startidx = cur_idx;
		for (Uint32 i = 0; i < mesh->mNumFaces; ++i) {
			aiFace* face = &mesh->mFaces[i];
			for (Uint32 j = 0; j < face->mNumIndices; ++j) {
			//for (Sint32 j = face->mNumIndices; j >= 0; --j) {
#if 0
				if (indexSize == 2) {
					Uint16* idx = (Uint16*)info->indices;
					idx[cur_idx] = (Uint16)face->mIndices[j] + startvtx;

				}
				else {
					Uint32* idx = (Uint32*)info->indices;
					idx[cur_idx] = face->mIndices[j] + startvtx;
				}
#endif
				cur_idx++;

				indices.push_back(face->mIndices[j] + startvtx);
			}
		}

		startvtx = cur_vtx;

		importinfo->info->mInfo[m].materialIdx = mesh->mMaterialIndex;
		importinfo->info->mInfo[m].indexOffset = startidx;
		importinfo->info->mInfo[m].indexCount = cur_idx - startidx;

	}

	// De-duplicate vertices

	std::vector<R3D_Vertex> new_vertices;
	std::vector<Uint32> new_indices;

	for (Uint32 i = 0; i < indices.size(); i++) {
		Uint32 idx = indices[i];
		R3D_Vertex vtx = vertices[idx];
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
	size_t idxSize = totalIndexCount * indexSize;

	importinfo->info->vertices = (R3D_Vertex*)rg_malloc(vtxSize);
	importinfo->info->indices = rg_malloc(idxSize);
	importinfo->info->vCount = totalVertexCount;
	importinfo->info->iCount = totalIndexCount;
	importinfo->info->iType = (IndexType)indexSize;

	// Copy de-duplicated vertex data

	for (Uint32 i = 0; i < new_vertices.size(); i++) {
		importinfo->info->vertices[i] = new_vertices[i];
	}

	if (indexSize == 2) {
		Uint16* idx = (Uint16*)importinfo->info->indices;
		for (Uint32 i = 0; i < new_indices.size(); i++) {
			idx[i] = (Uint16)new_indices[i];
		}

	}
	else {
		Uint32* idx = (Uint32*)importinfo->info->indices;
		for (Uint32 i = 0; i < new_indices.size(); i++) {
			idx[i] = new_indices[i];
		}
	}

	// Copy material data
	//info->matInfo

	R3D_MaterialInfo* mtl_array = (R3D_MaterialInfo*)rg_malloc(scene->mNumMaterials * sizeof(R3D_MaterialInfo));

	NameField* matnames_all = (NameField*)rg_malloc(sizeof(NameField) * scene->mNumMaterials);
	std::vector<R3D_MaterialInfo*> mtls;
	std::map<size_t, Uint32> mtl_hashtable;

	// Skip default material
	Uint32 matidx = 0;
	if (importinfo->skipFirstMat) {
		matidx = 1;
	}
	
	for (; matidx < scene->mNumMaterials; ++matidx) {
		aiMaterial* material = scene->mMaterials[matidx];

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
	importinfo->info->matCount = matcount;

	size_t matinfoSize = matcount * sizeof(R3D_MaterialInfo);
	importinfo->info->matInfo = (R3D_MaterialInfo*)rg_malloc(matinfoSize);

	importinfo->extra->mat_names = (NameField*)rg_malloc(sizeof(NameField) * matcount);

	for (Uint32 i = 0; i < scene->mNumMaterials; i++) {
		size_t mtl_hash = rgHash(&mtl_array[i], sizeof(R3D_MaterialInfo));
		Uint32 actual_idx = mtl_hashtable[mtl_hash];
		SDL_snprintf(importinfo->extra->mat_names[actual_idx].name, 128, "%s", matnames_all[i].name);
	}

	for (Uint32 i = 0; i < matcount; i++) {
		SDL_memcpy(&importinfo->info->matInfo[i], mtls[i], sizeof(R3D_MaterialInfo));
	}

	// and replace material index in meshes

	for (Uint32 i = 0; i < importinfo->info->mCount; i++) {
		Uint32 midx = importinfo->info->mInfo[i].materialIdx;

		size_t mtl_hash = rgHash(&mtl_array[midx], sizeof(R3D_MaterialInfo));
		Uint32 actual_idx = mtl_hashtable[mtl_hash];

		importinfo->info->mInfo[i].materialIdx = actual_idx;
	}


	size_t totalMemory = meshinfoSize + matinfoSize + vtxSize + idxSize;
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Used memory: %ldb", totalMemory);
	rgLogInfo(RG_LOG_SYSTEM, "[geom] Vertex buffer: vtx:%ldb idx:%ldb", vtxSize, idxSize);

	rg_free(mtl_array);
	rg_free(matnames_all);
	rg_free(transforms);

}

void FreeRiggedModel(R3DRiggedModelInfo* info, ModelExtraData* extra) {
	 
}

void ImportRiggedModel(ImportRiggedModelInfo* info) {

}
