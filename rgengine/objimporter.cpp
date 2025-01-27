#define DLL_EXPORT
#include "objimporter.h"

#include "filesystem.h"
#include "allocator.h"

#include "rgmath.h"

#include "render.h"
#include "engine.h"

#if 0
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <vector>
#include <map>
#endif

#if 1
// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

namespace Engine {

	static RG_INLINE void FetchTriangle(R3D_Vertex* vertices, Uint32* indices, Uint32 i, R3D_Vertex** v0, R3D_Vertex** v1, R3D_Vertex** v2) {
		Uint32 v0idx = indices[i * 3 + 0];
		Uint32 v1idx = indices[i * 3 + 1];
		Uint32 v2idx = indices[i * 3 + 2];
		*v0 = &vertices[v0idx];
		*v1 = &vertices[v1idx];
		*v2 = &vertices[v2idx];
	}

	static void RecalculateNormals(R3D_Vertex* vertices, Uint32* indices, size_t idx_count) {
		for (Uint32 i = 0; i < idx_count / 3; i++) {
			R3D_Vertex *v0, *v1, *v2;
			FetchTriangle(vertices, indices, i, &v0, &v1, &v2);

			vec3 v_0 = v1->pos - v0->pos;
			vec3 v_1 = v2->pos - v0->pos;

			vec3 N = v_0.cross(v_1).normalize();

			v0->norm = N;
			v1->norm = N;
			v2->norm = N;

		}
	}

	static void RecalculateTangents(R3D_Vertex* vertices, Uint32* indices, size_t idx_count) {
		for (Uint32 i = 0; i < idx_count / 3; i++) {
			R3D_Vertex* v0, * v1, * v2;
			FetchTriangle(vertices, indices, i, &v0, &v1, &v2);

			Float32 dx1 = v1->pos.x - v0->pos.x;
			Float32 dy1 = v1->pos.y - v0->pos.y;
			Float32 dz1 = v1->pos.z - v0->pos.z;
			Float32 dx2 = v2->pos.x - v0->pos.x;
			Float32 dy2 = v2->pos.y - v0->pos.y;
			Float32 dz2 = v2->pos.z - v0->pos.z;
			Float32 du1 = v1->uv.x  - v0->uv.x;
			Float32 dv1 = v1->uv.y  - v0->uv.y;
			Float32 du2 = v2->uv.x  - v0->uv.x;
			Float32 dv2 = v2->uv.y  - v0->uv.y;
			Float32 r = 1.0f / (du1 * dv2 - dv1 * du2);
			dx1 *= dv2;
			dy1 *= dv2;
			dz1 *= dv2;
			dx2 *= dv1;
			dy2 *= dv1;
			dz2 *= dv1;
			Float32 tx = (dx1 - dx2) * r;
			Float32 ty = (dy1 - dy2) * r;
			Float32 tz = (dz1 - dz2) * r;

			if (isnan(tx) || isnan(ty) || isnan(tz)) {
#if 1
				rgLogCritical(RG_LOG_SYSTEM, "Invalid face!");
				rgLogCritical(RG_LOG_SYSTEM, "v0 {%f %f %f}, {%f %f %f}, {%f %f}", v0->pos.x, v0->pos.y, v0->pos.z, v0->norm.x, v0->norm.y, v0->norm.z, v0->uv.x, v0->uv.y);
				rgLogCritical(RG_LOG_SYSTEM, "v1 {%f %f %f}, {%f %f %f}, {%f %f}", v1->pos.x, v1->pos.y, v1->pos.z, v1->norm.x, v1->norm.y, v1->norm.z, v1->uv.x, v1->uv.y);
				rgLogCritical(RG_LOG_SYSTEM, "v2 {%f %f %f}, {%f %f %f}, {%f %f}", v2->pos.x, v2->pos.y, v2->pos.z, v2->norm.x, v2->norm.y, v2->norm.z, v2->uv.x, v2->uv.y);
				rgLogCritical(RG_LOG_SYSTEM, "dUV1 %f %f, dUV2 %f %f, r %f", du1, dv1, du2, dv2, r);

				Uint32 v0idx = indices[i * 3 + 0];
				Uint32 v1idx = indices[i * 3 + 1];
				Uint32 v2idx = indices[i * 3 + 2];
				rgLogCritical(RG_LOG_SYSTEM, "vidx %d %d %d", v0idx, v1idx, v2idx);
#endif
#if 0
				tx = 1;
				ty = 0;
				tz = 0;
#endif
			}

			v0->tang.x = tx;
			v0->tang.y = ty;
			v0->tang.z = tz;
			v1->tang = v0->tang;
			v2->tang = v0->tang;
		}
	}

#if 0
	void ObjImporter::ImportModel(String p, R3DStaticModelInfo* info) {
		char file[256];
		SDL_memset(file, 0, 256);
		Engine::FS_ReplaceSeparators(file, p);

		char file_path[256];
		Engine::FS_PathFrom(file_path, file, 256, true);

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		rgLogInfo(RG_LOG_SYSTEM, "OBJ: Loading: %s", file);

		Bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, p, file_path);

		if (!err.empty()) {
			rgLogError(RG_LOG_SYSTEM, "%s", err.c_str());
		}

#if 1


		std::vector<R3D_Vertex> vertices;
		std::vector<Uint32> indices;

		std::vector<R3D_MatMeshInfo> matmeshes;

		Uint32 idx = 0;

		for (Uint32 i = 0; i < shapes.size(); i++) {

			// Process shape
			tinyobj::mesh_t mesh = shapes[i].mesh;

			for (size_t j = 0; j < mesh.indices.size(); j++) {

				// Process vertex

				tinyobj::index_t index = mesh.indices[j];
				Sint32 vidx = index.vertex_index;
				Sint32 nidx = index.normal_index;
				Sint32 tidx = index.texcoord_index;

				R3D_Vertex vtx = {};

				vtx.pos.x = attrib.vertices[vidx * 3 + 0];
				vtx.pos.y = attrib.vertices[vidx * 3 + 1];
				vtx.pos.z = attrib.vertices[vidx * 3 + 2];

				if (nidx != -1) {
					vtx.norm.x = attrib.normals[nidx * 3 + 0];
					vtx.norm.y = attrib.normals[nidx * 3 + 1];
					vtx.norm.z = attrib.normals[nidx * 3 + 2];
				}

				if (tidx != -1) {
					vtx.uv.x = attrib.texcoords[tidx * 2 + 0];
					vtx.uv.y = 1 - attrib.texcoords[tidx * 2 + 1];
				}


				vertices.push_back(vtx);
				indices.push_back(idx);
				idx++;
			}

		}

		R3D_MatMeshInfo mat_info = {};
		mat_info.indexOffset = 0;
		mat_info.indexCount  = indices.size();
		mat_info.materialIdx = 0;
		matmeshes.push_back(mat_info);

		IndexType indexType = RG_INDEX_U32;

		size_t index_count  = indices.size();
		size_t vertex_count = vertices.size();
		//size_t mesh_count = matmeshes.size();
		//size_t material_count = materials.size();
		size_t mesh_count     = 1;
		size_t material_count = 1;

		R3D_MaterialInfo* r3d_materials = (R3D_MaterialInfo*)rg_malloc(sizeof(R3D_MaterialInfo) * material_count);
		R3D_Vertex* r3d_vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * vertex_count);
		Uint32* r3d_indices = (Uint32*)rg_malloc(sizeof(Uint32) * index_count);
		R3D_MatMeshInfo* r3d_meshinfo = (R3D_MatMeshInfo*)rg_malloc(sizeof(R3D_MatMeshInfo) * mesh_count);

		// Copy data

		for (size_t i = 0; i < vertex_count; i++) {
			r3d_vertices[i] = vertices[i];
		}

		for (size_t i = 0; i < index_count; i++) {
			r3d_indices[i] = indices[i];
		}

		for (size_t i = 0; i < mesh_count; i++) {
			r3d_meshinfo[i] = matmeshes[i];
		}

		// Use default texture
		for (size_t i = 0; i < material_count; i++) {
			r3d_materials[i].color = { 1,1,1 };
			SDL_snprintf(r3d_materials[i].albedo, 128, "%s", "platform/textures/def_diffuse.png");
			SDL_snprintf(r3d_materials[i].normal, 128, "%s", "platform/textures/def_normal.png");
			SDL_snprintf(r3d_materials[i].pbr,    128, "%s", "platform/textures/def_pbr.png");
		}

		// TODO: Re/Calculate normals/tangents
		//RecalculateNormals(r3d_vertices, r3d_indices, index_count);
		RecalculateTangents(r3d_vertices, r3d_indices, index_count);


		// Final

		// Materials
		info->matInfo = r3d_materials;
		info->matCount = (Uint32)material_count;

		// Meshes
		info->mInfo = r3d_meshinfo;
		info->mCount = (Uint32)mesh_count;

		// Data
		info->vertices = r3d_vertices;
		info->vCount = (Uint32)vertex_count;
		info->indices = r3d_indices;
		info->iCount = (Uint32)index_count;
		info->iType = indexType;

#endif

#if 0

		// (Hash - vertex index) table
		std::map<size_t, Uint32> vtx_hashtable;

		std::vector<R3D_Vertex> vertices;
		std::vector<Uint32> indices;

		std::vector<R3D_MatMeshInfo> matmeshes;

		Uint32 prew_material = shapes[0].mesh.material_ids[0];
		Uint32 start_idx = 0;
		Uint32 end_idx = 0;

		R3D_MatMeshInfo mat_info = {};

		for (Uint32 i = 0; i < shapes.size(); i++) {
			tinyobj::shape_t* shape = &shapes[i];

			Uint32 idx_counter = 0;
			Uint32 verticesidx[4] = {}; // Up to 4 vertices per face


			Sint32 __vidx[4] = {};
			Sint32 __nidx[4] = {};
			Sint32 __tidx[4] = {};

			for (Uint32 j = 0; j < shape->mesh.num_face_vertices.size(); j++) {
				Uint32 vtx_per_face = shape->mesh.num_face_vertices[j];

				if (vtx_per_face < 3 || vtx_per_face > 4) {
					RG_ERROR_MSG("OBJ: Invalid polygon!");
				}

				for (Uint32 k = 0; k < vtx_per_face; k++) {

					tinyobj::index_t index = shape->mesh.indices[idx_counter];
					Sint32 vidx = index.vertex_index;
					Sint32 nidx = index.normal_index;
					Sint32 tidx = index.texcoord_index;

					__vidx[k] = vidx;
					__nidx[k] = nidx;
					__tidx[k] = tidx;

					R3D_Vertex vtx = {};

					vtx.pos.x = attrib.vertices[vidx * 3 + 0];
					vtx.pos.y = attrib.vertices[vidx * 3 + 1];
					vtx.pos.z = attrib.vertices[vidx * 3 + 2];

					if (nidx != -1) {
						vtx.norm.x = attrib.normals[nidx * 3 + 0];
						vtx.norm.y = attrib.normals[nidx * 3 + 1];
						vtx.norm.z = attrib.normals[nidx * 3 + 2];
					}

					if (tidx != -1) {
						vtx.uv.x = attrib.texcoords[tidx * 2 + 0];
						vtx.uv.y = 1 - attrib.texcoords[tidx * 2 + 1];
					}
				
					size_t vtx_hash = rgHash(&vtx, sizeof(R3D_Vertex));

					if (vtx_hashtable.count(vtx_hash) == 0) {
						// Add unique vertex
						vtx_hashtable[vtx_hash] = vertices.size();
						vertices.push_back(vtx);
					}

					idx_counter++;

					verticesidx[k] = vtx_hashtable[vtx_hash];

				}

				Uint32 face_material = shape->mesh.material_ids[j];
				if (face_material != prew_material) {

					mat_info.indexOffset = start_idx;
					mat_info.indexCount  = end_idx - start_idx;
					mat_info.materialIdx = prew_material;

					matmeshes.push_back(mat_info);

					start_idx = end_idx;
					//end_idx = 0;
					prew_material = face_material;

				}

				R3D_Vertex v_0 = vertices[verticesidx[0]];
				R3D_Vertex v_1 = vertices[verticesidx[1]];
				R3D_Vertex v_2 = vertices[verticesidx[2]];
				if (v_0.pos == v_1.pos || v_0.pos == v_2.pos || v_1.pos == v_2.pos) {
					rgLogError(RG_LOG_SYSTEM, "OBJ: Invalid face: (%d %d %d) at: %s s:%d f:%d",
						verticesidx[0],
						verticesidx[1],
						verticesidx[2],
						shape->name.c_str(), i, j);

					rgLogError(RG_LOG_SYSTEM, "OBJ: + (%d %d %d) (%d %d %d) (%d %d %d)",
						__vidx[0], __nidx[0], __tidx[0],
						__vidx[1], __nidx[1], __tidx[1],
						__vidx[2], __nidx[2], __tidx[2]
					);

					rgLogCritical(RG_LOG_SYSTEM, "v0 {%f %f %f}, {%f %f %f}, {%f %f}", v_0.pos.x, v_0.pos.y, v_0.pos.z, v_0.norm.x, v_0.norm.y, v_0.norm.z, v_0.uv.x, v_0.uv.y);
					rgLogCritical(RG_LOG_SYSTEM, "v1 {%f %f %f}, {%f %f %f}, {%f %f}", v_1.pos.x, v_1.pos.y, v_1.pos.z, v_1.norm.x, v_1.norm.y, v_1.norm.z, v_1.uv.x, v_1.uv.y);
					rgLogCritical(RG_LOG_SYSTEM, "v2 {%f %f %f}, {%f %f %f}, {%f %f}", v_2.pos.x, v_2.pos.y, v_2.pos.z, v_2.norm.x, v_2.norm.y, v_2.norm.z, v_2.uv.x, v_2.uv.y);
				}

				if (vtx_per_face == 3) {
					// Triangle
					indices.push_back(verticesidx[0]);
					indices.push_back(verticesidx[1]);
					indices.push_back(verticesidx[2]);
					end_idx += 3;
				} else {
					// Quad (2 triangles)
					indices.push_back(verticesidx[0]);
					indices.push_back(verticesidx[1]);
					indices.push_back(verticesidx[2]);

					indices.push_back(verticesidx[2]);
					indices.push_back(verticesidx[3]);
					indices.push_back(verticesidx[0]);

					end_idx += 6;
				}

			}
		}

		mat_info.indexOffset = start_idx;
		mat_info.indexCount  = end_idx - start_idx;
		mat_info.materialIdx = prew_material;
		matmeshes.push_back(mat_info);


		IndexType indexType = RG_INDEX_U32;

		size_t index_count    = indices.size();
		size_t vertex_count   = vertices.size();
		size_t mesh_count     = matmeshes.size();
		size_t material_count = materials.size();

		R3D_MaterialInfo* r3d_materials = (R3D_MaterialInfo*)rg_malloc(sizeof(R3D_MaterialInfo) * material_count);
		R3D_Vertex*       r3d_vertices  = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * vertex_count);
		Uint32*           r3d_indices   = (Uint32*)rg_malloc(sizeof(Uint32) * index_count);
		R3D_MatMeshInfo*  r3d_meshinfo  = (R3D_MatMeshInfo*)rg_malloc(sizeof(R3D_MatMeshInfo) * mesh_count);

		// Copy data

		for (size_t i = 0; i < vertex_count; i++) {
			r3d_vertices[i] = vertices[i];
		}

		for (size_t i = 0; i < index_count; i++) {
			r3d_indices[i] = indices[i];
		}

		for (size_t i = 0; i < mesh_count; i++) {
			r3d_meshinfo[i] = matmeshes[i];
		}

		for (size_t i = 0; i < material_count; i++) {

			r3d_materials[i].color = { 1,1,1 };
			if (!materials[i].diffuse_texname.empty()) {
				SDL_snprintf(r3d_materials[i].albedo, 128, "%s%s", file_path, materials[i].diffuse_texname.c_str());
			} else {
				SDL_snprintf(r3d_materials[i].albedo, 128, "%s", "platform/textures/def_diffuse.png");
			}
			
			SDL_snprintf(r3d_materials[i].normal, 128, "%s", "platform/textures/def_normal.png");
			SDL_snprintf(r3d_materials[i].pbr, 128, "%s", "platform/textures/def_pbr.png");

		}

		// TODO: Re/Calculate normals/tangents
		RecalculateNormals(r3d_vertices, r3d_indices, index_count);
		RecalculateTangents(r3d_vertices, r3d_indices, index_count);


		// Final

		// Materials
		info->matInfo  = r3d_materials;
		info->matCount = (Uint32)material_count;

		// Meshes
		info->mInfo  = r3d_meshinfo;
		info->mCount = (Uint32)mesh_count;

		// Data
		info->vertices = r3d_vertices;
		info->vCount   = (Uint32)vertex_count;
		info->indices  = r3d_indices;
		info->iCount   = (Uint32)index_count;
		info->iType    = indexType;

#endif

	}

	void ObjImporter::FreeModelData(R3DStaticModelInfo* info) {
#if 1
		rg_free(info->vertices);
		rg_free(info->indices);
		rg_free(info->mInfo);
		rg_free(info->matInfo);
#endif
	}
#endif
#if 1

	void ObjImporter::ImportModel(String p, R3DStaticModelInfo* info) {

		char path[256];
		SDL_memset(path, 0, 256);
		Engine::FS_ReplaceSeparators(path, p);

		char GAMEDATA_PATH[256];
		Engine::FS_PathFrom(GAMEDATA_PATH, path, 256);

		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(path,
			aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_GenNormals  |
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

		R3D_MaterialInfo* materials = (R3D_MaterialInfo*)rg_malloc((scene->mNumMaterials - 1) * sizeof(R3D_MaterialInfo));

		char new_path[128];
		char n_new_path[128];

		// skip first assimp material
		// assimp's materials -> [default unused, mat0, mat1, mat2, ...]
		// engine's materials -> [mat0, mat1, mat2, ...]

		for (Uint32 i = 0; i < scene->mNumMaterials - 1; i++) {
			aiMaterial* mat = scene->mMaterials[i + 1];

			//aiColor4D diffuse;
			//ai_real d;
			//aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
			//aiGetMaterialFloat(mat, AI_MATKEY_OPACITY, &d);

			aiString ai_d_str;
			aiString ai_n_str;
			aiGetMaterialTexture(mat, aiTextureType_DIFFUSE, 0, &ai_d_str);
			aiGetMaterialTexture(mat, aiTextureType_DISPLACEMENT, 0, &ai_n_str);

			aiColor4D color;
			aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color);

			SDL_memset(new_path, 0, 128);
			SDL_memset(n_new_path, 0, 128);

			if (ai_d_str.length) {
				FixPath(new_path, ai_d_str.C_Str());
				SDL_snprintf(materials[i].texture, 128, "%s", new_path); // TODO: remove file extension
			} else {
				SDL_snprintf(materials[i].texture, 128, "default");
			}
#if 0
			if (ai_n_str.length != 0) {
				FixPath(n_new_path, ai_n_str.C_Str());
				SDL_snprintf(materials[i].normal, 128, "%s/%s", GAMEDATA_PATH, n_new_path);
			} else {
				SDL_snprintf(materials[i].normal, 128, "%s/textures/def_normal.png", GetPlatformPath());
			}
#endif

#if 0
			SDL_snprintf(materials[i].pbr, 128, "%s/textures/def_pbr.png", GetPlatformPath());
			materials[i].color = {1, 1, 1};
			materials[i].color.r = color.r;
			materials[i].color.g = color.g;
			materials[i].color.b = color.b;
			//materials[i].color.a = color.a;
#endif
		}

		R3D_MatMeshInfo* minfo = (R3D_MatMeshInfo*)rg_malloc(scene->mNumMeshes * sizeof(R3D_MatMeshInfo));
		Uint32 vtx = 0;
		Uint32 idx = 0;

		mat4 rotation_matrix = MAT4_IDENTITY();

		Uint32 idx_offset = 0;

		AABB aabb = { {10000, 10000, 10000}, {-10000, -10000, -10000} };

		for (Uint32 i = 0; i < scene->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[i];
			Uint32 offset = vtx;
			Bool a = false;

			for (Uint32 j = 0; j < mesh->mNumVertices; j++) {
				vec4 p_vec = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z, 1 };
				vec4 n_vec = {};
				if (mesh->mNormals) {
					n_vec = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z, 0 };
				}
				vec4 t_vec = {};
				if (mesh->mTangents) {
					t_vec = { mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z, 0 };
				}
				vec4 np_vec = rotation_matrix * p_vec;
				vec4 nn_vec = rotation_matrix * n_vec;
				vec4 nt_vec = rotation_matrix * t_vec;

				if (np_vec.x < aabb.min.x) { aabb.min.x = np_vec.x; }
				if (np_vec.y < aabb.min.y) { aabb.min.y = np_vec.y; }
				if (np_vec.z < aabb.min.z) { aabb.min.z = np_vec.z; }
				if (np_vec.x > aabb.max.x) { aabb.max.x = np_vec.x; }
				if (np_vec.y > aabb.max.y) { aabb.max.y = np_vec.y; }
				if (np_vec.z > aabb.max.z) { aabb.max.z = np_vec.z; }


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


			Uint32 sidx = idx;
			for (Uint32 j = 0; j < mesh->mNumFaces; j++) {
				indices[idx + 0] = offset + mesh->mFaces[j].mIndices[0];
				indices[idx + 1] = offset + mesh->mFaces[j].mIndices[1];
				indices[idx + 2] = offset + mesh->mFaces[j].mIndices[2];
				idx += 3;
			}

			// Calculate nomrlas if needed
			//if (!mesh->mNormals) {
			//	Uint32* ptr = &indices[sidx];
			//	RecalculateNormals(vertices, ptr, mesh->mNumFaces * 3);
			//}

			//RecalculateTangetns(mesh->mNumVertices, vertices, idxoffset, mesh->mNumFaces*3, indices);

			minfo[i].indexCount  = mesh->mNumFaces * 3;
			minfo[i].indexOffset = idx_offset;
			minfo[i].materialIdx = mesh->mMaterialIndex - 1;

			idx_offset += minfo[i].indexCount;
		}

		// Regen normals
		//RecalculateNormals(vertices, indices, index_count);

		// Materials
		info->matInfo  = materials;
		info->matCount = scene->mNumMaterials - 1;

		// Meshes
		info->mInfo  = minfo;
		info->mCount = scene->mNumMeshes;

		// Data
		info->vertices = vertices;
		info->vCount   = vertex_count;
		info->indices  = indices;
		info->iCount   = index_count;
		info->iType    = RG_INDEX_U32;

		info->aabb     = aabb;


		importer.FreeScene();
	}

	void ObjImporter::FreeModelData(R3DStaticModelInfo* info) {
#if 1
		rg_free(info->vertices);
		rg_free(info->indices);
		rg_free(info->mInfo);
		rg_free(info->matInfo);
#endif
	}

#endif
}