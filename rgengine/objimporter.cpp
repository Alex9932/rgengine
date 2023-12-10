#include "objimporter.h"

#include "filesystem.h"
#include "allocator.h"

#include "rgmath.h"

#include "render.h"

// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace Engine {
	namespace Render {

		void ObjImporter::ImportModel(String path, R3DCreateStaticModelInfo* info) {

			char GAMEDATA_PATH[256];
			Engine::FS_PathFrom(GAMEDATA_PATH, path, 256);

			Assimp::Importer importer;

			const aiScene* scene = importer.ReadFile(path,
				aiProcess_Triangulate |
				aiProcess_FlipUVs |
				//aiProcess_GenNormals  |
				aiProcess_CalcTangentSpace
			);

			Uint32 vertex_count = 0;
			Uint32 index_count = 0;
			for (Uint32 i = 0; i < scene->mNumMeshes; i++) {
				vertex_count += scene->mMeshes[i]->mNumVertices;
				index_count += scene->mMeshes[i]->mNumFaces * 3; // Triangle faces
			}

			R3D_Vertex* vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * vertex_count);
			Uint32* indices = (Uint32*)rg_malloc(sizeof(Uint32) * index_count);

			R3D_Material** materials = (R3D_Material**)rg_malloc(scene->mNumMaterials * sizeof(R3D_Material*));

			char str[256];
			char new_path[256];
			char n_str[256];
			char n_new_path[256];

			for (Uint32 i = 0; i < scene->mNumMaterials; i++) {
				aiMaterial* mat = scene->mMaterials[i];

				aiString ai_d_str;
				aiString ai_n_str;
				aiColor4D diffuse;
				ai_real d;
				aiGetMaterialTexture(mat, aiTextureType_DIFFUSE, 0, &ai_d_str);
				aiGetMaterialTexture(mat, aiTextureType_DISPLACEMENT, 0, &ai_n_str);
				aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
				aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &d);

				String cstr = ai_d_str.C_Str();
				String n_cstr = ai_n_str.C_Str();
				size_t len = SDL_strlen(cstr);
				size_t n_len = SDL_strlen(n_cstr);
				size_t j = 0;
				SDL_memset(str, 0, 256);
				SDL_memset(new_path, 0, 256);
				SDL_memset(n_str, 0, 256);
				SDL_memset(n_new_path, 0, 256);

				if (len != 0) {
					for (size_t k = 0; k < len; k++) {
						if (k != 0 && cstr[k - 1] == '\\' && cstr[k] == '\\') { continue; }
						if (cstr[k] == '\\') {
							new_path[j] = '/';
							j++;
							continue;
						}
						new_path[j] = cstr[k];
						j++;
					}

					SDL_snprintf(str, 256, "%s/%s", GAMEDATA_PATH, new_path);
				}
				else {
					SDL_snprintf(str, 256, "platform/textures/def_diffuse.png");
				}

				j = 0;
				if (n_len != 0) {
					for (size_t k = 0; k < n_len; k++) {
						if (k != 0 && n_cstr[k - 1] == '\\' && n_cstr[k] == '\\') { continue; }
						if (n_cstr[k] == '\\') {
							n_new_path[j] = '/';
							j++;
							continue;
						}
						n_new_path[j] = n_cstr[k];
						j++;
					}

					SDL_snprintf(n_str, 256, "%s/%s", GAMEDATA_PATH, n_new_path);
				}
				else {
					SDL_snprintf(n_str, 256, "platform/textures/def_normal.png");
				}

				R3DCreateMaterialInfo matInfo = {};
				matInfo.albedo = str;
				matInfo.normal = n_str;
				matInfo.pbr = "platform/textures/def_pbr.png";
				matInfo.color = { 1, 1, 1 };
				materials[i] = Render::R3D_CreateMaterial(&matInfo);

			}

			R3D_MeshInfo* minfo = (R3D_MeshInfo*)rg_malloc(scene->mNumMeshes * sizeof(R3D_MeshInfo));
			Uint32 vtx = 0;
			Uint32 idx = 0;

			mat4 rotation_matrix = MAT4_IDENTITY();

			for (Uint32 i = 0; i < scene->mNumMeshes; i++) {
				aiMesh* mesh = scene->mMeshes[i];
				Uint32 offset = vtx;
				bool a = false;

				for (Uint32 j = 0; j < mesh->mNumVertices; j++) {
					vec4 p_vec = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z, 1 };
					vec4 n_vec = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z, 0 };
					vec4 t_vec = { mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z, 0 };
					vec4 np_vec = rotation_matrix * p_vec;
					vec4 nn_vec = rotation_matrix * n_vec;
					vec4 nt_vec = rotation_matrix * t_vec;
					vertices[vtx].pos.x = np_vec.x;
					vertices[vtx].pos.y = np_vec.y;
					vertices[vtx].pos.z = np_vec.z;
					vertices[vtx].norm.x = nn_vec.x;
					vertices[vtx].norm.y = nn_vec.y;
					vertices[vtx].norm.z = nn_vec.z;
					vertices[vtx].tang.x = nt_vec.x;
					vertices[vtx].tang.y = nt_vec.y;
					vertices[vtx].tang.z = nt_vec.z;

					if (mesh->mTextureCoords[0] != NULL) {
						vertices[vtx].uv.x = mesh->mTextureCoords[0][j].x;
						vertices[vtx].uv.y = mesh->mTextureCoords[0][j].y;
					}
					else {
						if (!a) {
							rgLogWarn(RG_LOG_RENDER, " %d -> No texture", i);
							a = true;
						}
						vertices[vtx].uv.x = 0;
						vertices[vtx].uv.y = 0;
					}
					vtx++;
				}


				//Uint32 idxoffset = idx;
				for (Uint32 j = 0; j < mesh->mNumFaces; j++) {
					indices[idx + 0] = offset + mesh->mFaces[j].mIndices[0];
					indices[idx + 1] = offset + mesh->mFaces[j].mIndices[1];
					indices[idx + 2] = offset + mesh->mFaces[j].mIndices[2];
					idx += 3;
				}

				//RecalculateTangetns(mesh->mNumVertices, vertices, idxoffset, mesh->mNumFaces*3, indices);

				minfo[i].indexCount = mesh->mNumFaces * 3;
				minfo[i].material = materials[mesh->mMaterialIndex];
			}

			info->vertices = vertices;
			info->vCount = vertex_count;
			info->indices = indices;
			info->iCount = index_count;
			info->iType = RG_INDEX_U32;
			info->info = minfo;
			info->mCount = scene->mNumMeshes;

			rg_free(materials);

			importer.FreeScene();
		}

		void ObjImporter::FreeModelData(R3DCreateStaticModelInfo* info) {
#if 0
			rg_free(info->vertices);
			rg_free(info->indices);
			rg_free(info->info);
#endif
		}

	}
}