/*
 * pmd.cpp
 *
 *  Created on: Mar 20, 2022
 *      Author: alex9932
 */

#define DLL_EXPORT

#include "pmd.h"

#ifndef STD_PMD

#include <cstdio>

#include <allocator.h>
#include <filesystem.h>
#include <engine.h>
#include <utf8.h>

#define PMD_PRINTINFO

pmd_file* pmd_load(String file) {

	Engine::FSReader* reader = new Engine::FSReader(file);

	rgLogInfo(RG_LOG_SYSTEM, "PMD: Loading model: %s\n", file);

	pmd_file* pmd = (pmd_file*)rg_malloc(sizeof(pmd_file));
	pmd->is_extended = false;

	reader->Read(pmd->signature, 3);
	pmd->version = reader->ReadF32();

	if (pmd->signature[0] != 'P' ||
		pmd->signature[1] != 'm' ||
		pmd->signature[2] != 'd') {
		char buffer[128];
		SDL_snprintf(buffer, 128, "%s is not a Polygon Model Data file!", file);
		RG_ERROR_MSG(buffer);
	}


	if(pmd->version != 1.0f) {
		rgLogWarn(RG_LOG_SYSTEM, "PMD v%f is not supported!", pmd->version);
	}

	char path[256] = {};
	Engine::FS_ReplaceSeparators(path, file);
	Engine::FS_PathFrom(pmd->path, path, 256);

	reader->Read(pmd->name, 20);
	reader->Read(pmd->comment, 256);

	if (Engine::IsDebug()) {
		rgLogWarn(RG_LOG_SYSTEM, "PMD: Sig %s", pmd->signature);
		UTF8_FromSJIS(pmd->name);
		rgLogWarn(RG_LOG_SYSTEM, "PMD: Name %s", UTF8_GetBuffer());
		UTF8_FromSJIS(pmd->comment);
		rgLogWarn(RG_LOG_SYSTEM, "PMD: Comment %s", UTF8_GetBuffer());
	}

	pmd->vertex_count = reader->ReadU32();

	pmd->vertices = (pmd_vertex*)rg_malloc(sizeof(pmd_vertex) * pmd->vertex_count);
	for (Uint32 i = 0; i < pmd->vertex_count; ++i) {
		reader->Read(&pmd->vertices[i].vertex, sizeof(pmd_position));
		pmd->vertices[i].weight.b_id[0] = reader->ReadU16();
		pmd->vertices[i].weight.b_id[1] = reader->ReadU16();

		Uint16 w = reader->ReadU8();
		pmd->vertices[i].weight.b_weight[0] = w;
		pmd->vertices[i].weight.b_weight[1] = 100 - w;
		pmd->vertices[i].edge = reader->ReadU8();

#if 0
		// TODO: !!! THIS IS EDGE FLAG NOT BONE WEIGHT !!!
		// REWRITE THIS
		pmd->vertices[i].weight.b_weight[1] = reader->ReadU8();
#endif

	}

	pmd->index_count = reader->ReadU32();

	pmd->indices = (Uint16*)rg_malloc(sizeof(Uint16) * pmd->index_count);
	reader->Read(pmd->indices, sizeof(Uint16) * pmd->index_count);
	pmd->material_count = reader->ReadU32();

	pmd->materials = (pmd_material*)rg_malloc(sizeof(pmd_material) * pmd->material_count);
	for (Uint32 i = 0; i < pmd->material_count; ++i) {
//		rgLogInfo(RG_LOG_SYSTEM, "Mat %d at 0x%x", i, reader->GetOffset());
		reader->Read(&pmd->materials[i].colors, sizeof(pmd_material_colors));
		pmd->materials[i].toon_number = reader->ReadU8();
		pmd->materials[i].edge_flag = reader->ReadU8();
		pmd->materials[i].surface_count = reader->ReadU32();
		reader->Read(pmd->materials[i].file_name, 20);
	}

	char char_buffer[20];
	pmd->bones_count = reader->ReadU16();

	pmd->bones = (pmd_bone*)rg_malloc(sizeof(pmd_bone) * pmd->bones_count);
	Uint8 u8;
	for (Uint32 i = 0; i < pmd->bones_count; ++i) {
		reader->Read(char_buffer, 20);
		UTF8_FromSJIS(char_buffer);
		SDL_memset(pmd->bones[i].name, 0, 32);
		SDL_memcpy(pmd->bones[i].name, UTF8_GetBuffer(), SDL_strlen(UTF8_GetBuffer()));
		pmd->bones[i].parent = reader->ReadU16();
		pmd->bones[i].child = reader->ReadU16();
		pmd->bones[i].bone_type = reader->ReadU8();
		pmd->bones[i].bone_target = reader->ReadU16();
		reader->Read3F32(pmd->bones[i].position);
	}

	pmd->ik_count = reader->ReadU16();

	pmd->ik = (pmd_ik_info*)rg_malloc(sizeof(pmd_ik_info) * pmd->ik_count);

	for (Uint32 i = 0; i < pmd->ik_count; ++i) {
		pmd->ik[i].target = reader->ReadU16();
		pmd->ik[i].effector = reader->ReadU16();
		pmd->ik[i].bones = reader->ReadU8();
		pmd->ik[i].max_iterations = reader->ReadU16();
		pmd->ik[i].angle_limit = reader->ReadF32();
		pmd->ik[i].list = (Uint16*)rg_malloc(sizeof(Uint16) * pmd->ik[i].bones);
		reader->Read(pmd->ik[i].list, sizeof(Uint16) * pmd->ik[i].bones);

		if (Engine::IsDebug()) {
			rgLogInfo(RG_LOG_SYSTEM, "PMD: IK chain[%d] %d, %d %d", i, pmd->ik[i].bones, pmd->ik[i].effector, pmd->ik[i].target);
		}
	}

	pmd->morph_count = reader->ReadU16();
	pmd->morphs = (pmd_morph*)rg_malloc(sizeof(pmd_morph) * pmd->morph_count);

	for (Uint32 i = 0; i < pmd->morph_count; ++i) {
		reader->Read(char_buffer, 20);
		UTF8_FromSJIS(char_buffer);
		SDL_memset(pmd->morphs[i].name, 0, 32);
		SDL_memcpy(pmd->morphs[i].name, UTF8_GetBuffer(), SDL_strlen(UTF8_GetBuffer()));
		pmd->morphs[i].vertices = reader->ReadU32();
		pmd->morphs[i].face_type = reader->ReadU8();
		pmd->morphs[i].list = (pmd_morph_vertex*)rg_malloc(sizeof(pmd_morph_vertex) * pmd->morphs[i].vertices);
		reader->Read(pmd->morphs[i].list, sizeof(pmd_morph_vertex) * pmd->morphs[i].vertices);
	}

	// Faces
	u8 = reader->ReadU8();
	for (Uint8 i = 0; i < u8; ++i) {
		reader->ReadU16();
	}

	// Bone group names
	char group[50];
	pmd->bone_groups = reader->ReadU8();
	for (Uint8 i = 0; i < pmd->bone_groups; ++i) {
		reader->Read(group, 50);
	}

	// Displayed bones
	Uint32 idx = reader->ReadU32();
	for (Uint32 i = 0; i < idx; ++i) {
		reader->ReadU16();
		reader->ReadU8();
	}

	///////////////////////////////////////////
	//                                       //
	//  !!! END OF THE BASE FILE FORMAT !!!  //
	//                                       //
	///////////////////////////////////////////

	u8 = reader->ReadU8();
	if(!reader->EndOfStream()) {
		pmd->is_extended = true;
	}

	// English data information
	if(pmd->is_extended && u8) {
		reader->Read(pmd->e_name, 20);
		reader->Read(pmd->e_comment, 256);

		if (Engine::IsDebug()) {
			rgLogInfo(RG_LOG_SYSTEM, "PMD: ~ ~ ~ EXTENDED DATA ~ ~ ~\n");
			rgLogInfo(RG_LOG_SYSTEM, "PMD: Model name: %s\n", pmd->e_name);
			rgLogInfo(RG_LOG_SYSTEM, "PMD: Comment: %s\n", pmd->e_comment);
		}

		// TODO: Rewrite memalloc
		pmd->e_bones = (char**)rg_malloc(sizeof(char*) * pmd->bones_count);
		pmd->e_faces = (char**)rg_malloc(sizeof(char*) * pmd->morph_count);
		pmd->e_bone_group = (char**)rg_malloc(sizeof(char*) * pmd->bone_groups);
		pmd->e_toon_textures = (char**)rg_malloc(sizeof(char*) * 10);

		for (Uint32 i = 0; i < pmd->bones_count; ++i) {
			pmd->e_bones[i] = (char*)rg_malloc(20);
			reader->Read(pmd->e_bones[i], 20);
		}

		for (Uint32 i = 0; i < pmd->morph_count - 1; ++i) {
			pmd->e_faces[i] = (char*)rg_malloc(20);
			reader->Read(pmd->e_faces[i], 20);
		}

		for (Uint32 i = 0; i < pmd->bone_groups; ++i) {
			pmd->e_bone_group[i] = (char*)rg_malloc(50);
			reader->Read(pmd->e_bone_group[i], 50);
		}

		for (Uint32 i = 0; i < 10; ++i) {
			pmd->e_toon_textures[i] = (char*)rg_malloc(100);
			reader->Read(pmd->e_toon_textures[i], 100);
		}

		pmd->rigidbody_count = reader->ReadU32();

		if(reader->EndOfStream()) {
			delete reader;
			return pmd;
		}

		if(pmd->rigidbody_count != 0) {
			pmd->rigidbodies = (pmd_rigidbody*)rg_malloc(sizeof(pmd_rigidbody) * pmd->rigidbody_count);
			for (Uint32 i = 0; i < pmd->rigidbody_count; ++i) {
				reader->Read(pmd->rigidbodies[i].name, 20);
				pmd->rigidbodies[i].bone_id = reader->ReadU16();
				pmd->rigidbodies[i].collision_group = reader->ReadU8();
				pmd->rigidbodies[i].collision_group_mask = reader->ReadU16();
				pmd->rigidbodies[i].collision_shape = reader->ReadU8();
				pmd->rigidbodies[i].w = reader->ReadF32();
				pmd->rigidbodies[i].h = reader->ReadF32();
				pmd->rigidbodies[i].d = reader->ReadF32();
				pmd->rigidbodies[i].x = reader->ReadF32();
				pmd->rigidbodies[i].y = reader->ReadF32();
				pmd->rigidbodies[i].z = reader->ReadF32();
				pmd->rigidbodies[i].rx = reader->ReadF32();
				pmd->rigidbodies[i].ry = reader->ReadF32();
				pmd->rigidbodies[i].rz = reader->ReadF32();
				pmd->rigidbodies[i].mass = reader->ReadF32();
				pmd->rigidbodies[i].linear_dampening = reader->ReadF32();
				pmd->rigidbodies[i].angular_dampening = reader->ReadF32();
				pmd->rigidbodies[i].restitution_coefficient = reader->ReadF32();
				pmd->rigidbodies[i].friction_coefficient = reader->ReadF32();
				pmd->rigidbodies[i].body_type = reader->ReadU8();
			}
		}

		pmd->constraint_count = reader->ReadU32();
		if(pmd->constraint_count != 0) {
			pmd->constraints = (pmd_constraint*)rg_malloc(sizeof(pmd_constraint) * pmd->constraint_count);
			reader->Read(pmd->constraints, sizeof(pmd_constraint) * pmd->constraint_count);
		}

		if (Engine::IsDebug()) {
			rgLogInfo(RG_LOG_SYSTEM, "IK chains: %d, Morphs: %d\n", pmd->ik_count, pmd->morph_count);
			rgLogInfo(RG_LOG_SYSTEM, "PMD: Bodies: %d, constraints: %d\n", pmd->rigidbody_count, pmd->constraint_count);
		}
	}

	delete reader;
	return pmd;
}

void pmd_free(pmd_file* ptr) {
	rg_free(ptr->vertices);
	rg_free(ptr->indices);
	rg_free(ptr->materials);
	rg_free(ptr->bones);
	for (Uint32 i = 0; i < ptr->ik_count; ++i) {
		rg_free(ptr->ik[i].list);
	}
	for (Uint32 i = 0; i < ptr->morph_count; ++i) {
		rg_free(ptr->morphs[i].list);
	}
	rg_free(ptr->ik);
	rg_free(ptr->morphs);
	//if(ptr->is_extended) {
	//	for (Uint32 i = 0; i < ptr->bones_count; ++i) {
	//		rg_free(ptr->e_bones[i]);
	//	}
	//	for (Uint32 i = 0; i < ptr->morph_count; ++i) {
	//		rg_free(ptr->e_faces[i]);
	//	}
	//	for (Uint32 i = 0; i < ptr->bone_groups; ++i) {
	//		rg_free(ptr->e_bone_group[i]);
	//	}
	//	for (Uint32 i = 0; i < 10; ++i) {
	//		rg_free(ptr->e_toon_textures[i]);
	//	}
	//	rg_free(ptr->rigidbodies);
	//	rg_free(ptr->constraints);
	//}
	rg_free(ptr);
}

#endif
