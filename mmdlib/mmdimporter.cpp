#define DLL_EXPORT
#include "mmdimporter.h"

#define RG_PMD_RECALCULATE_TANGENTS 0

#include <allocator.h>
#include <kinematicsmodel.h>
#include "pmd.h"
#include "vmd.h"

#include <render.h>
#include <filesystem.h>

using namespace Engine;

static void LoadPMD(String p, pmd_file** pmd_ptr, R3D_Vertex** vtx, Uint16** idx) {

	char file[256];
	SDL_memset(file, 0, 256);
	Engine::FS_ReplaceSeparators(file, p);

	pmd_file* pmd = pmd_load(file);
	*pmd_ptr = pmd;

	// Load vertices
	Uint16* indices = (Uint16*)rg_malloc(pmd->index_count * sizeof(Uint16));
	SDL_memcpy(indices, pmd->indices, sizeof(Uint16) * pmd->index_count);

	R3D_Vertex* vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * pmd->vertex_count);
	for (Uint32 i = 0; i < pmd->vertex_count; i++) {
		vertices[i].pos.x  = pmd->vertices[i].vertex.x;
		vertices[i].pos.y  = pmd->vertices[i].vertex.y;
		vertices[i].pos.z  = pmd->vertices[i].vertex.z;
		vertices[i].norm.x = pmd->vertices[i].vertex.nx;
		vertices[i].norm.y = pmd->vertices[i].vertex.ny;
		vertices[i].norm.z = pmd->vertices[i].vertex.nz;
		vertices[i].tang.x = 0;
		vertices[i].tang.y = 0;
		vertices[i].tang.z = 1;
		vertices[i].uv.x   = pmd->vertices[i].vertex.u;
		vertices[i].uv.y   = pmd->vertices[i].vertex.v;
	}

#if RG_PMD_RECALCULATE_TANGENTS
	// Calculate tangents
	for (Uint32 i = 0; i < pmd->index_count / 3; i += 3) {
		Uint32 v0idx = pmd->indices[i + 0];
		Uint32 v1idx = pmd->indices[i + 1];
		Uint32 v2idx = pmd->indices[i + 2];
		R3D_Vertex* v0 = &vertices[v0idx];
		R3D_Vertex* v1 = &vertices[v1idx];
		R3D_Vertex* v2 = &vertices[v2idx];
		Float32 dx1 = v1->pos.x - v0->pos.x;
		Float32 dy1 = v1->pos.y - v0->pos.y;
		Float32 dz1 = v1->pos.z - v0->pos.z;
		Float32 dx2 = v2->pos.x - v0->pos.x;
		Float32 dy2 = v2->pos.y - v0->pos.y;
		Float32 dz2 = v2->pos.z - v0->pos.z;
		Float32 du1 = v1->uv.x - v0->uv.x;
		Float32 dv1 = v1->uv.y - v0->uv.y;
		Float32 du2 = v2->uv.x - v0->uv.x;
		Float32 dv2 = v2->uv.y - v0->uv.y;
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
		v0->tang.x = tx;
		v0->tang.y = ty;
		v0->tang.z = tz;
		v1->tang = v0->tang;
		v2->tang = v0->tang;
	}
#endif

	*vtx = vertices;
	*idx = indices;

}

static void LoadPMDMaterials(pmd_file* pmd, R3D_MaterialInfo** info, R3D_MatMeshInfo** meshinfo) {

	// Load materials (rewrite this)
	R3D_MaterialInfo* matsInfo = (R3D_MaterialInfo*)rg_malloc(sizeof(R3D_MaterialInfo) * pmd->material_count);
	R3D_MatMeshInfo*  mmInfo   = (R3D_MatMeshInfo*)rg_malloc(sizeof(R3D_MatMeshInfo) * pmd->material_count);

	Float32 colorMul = 2;

	for (Uint32 i = 0; i < pmd->material_count; i++) {
		pmd_material* mat = &pmd->materials[i];
		if (mat->file_name[0] == '\0') {
			SDL_snprintf(matsInfo[i].albedo, 128, "platform/toon/toon%02d.png", mat->toon_number);
		} else {
			SDL_snprintf(matsInfo[i].albedo, 128, "%s/%s", pmd->path, mat->file_name);
		}

		SDL_snprintf(matsInfo[i].normal, 128, "platform/textures/def_normal.png");
		SDL_snprintf(matsInfo[i].pbr, 128, "platform/textures/def_pbr.png");

		matsInfo[i].color.r = mat->colors.r * colorMul;
		matsInfo[i].color.g = mat->colors.g * colorMul;
		matsInfo[i].color.b = mat->colors.b * colorMul;

		mmInfo[i].indexCount  = mat->surface_count;
		mmInfo[i].materialIdx = i;
	}

	*info     = matsInfo;
	*meshinfo = mmInfo;
}

static void LoadPMDWeights(pmd_file* pmd, R3D_Weight** w) {
	R3D_Weight* weights = (R3D_Weight*)rg_malloc(sizeof(R3D_Weight) * pmd->vertex_count);

	for (Uint32 i = 0; i < pmd->vertex_count; i++) {
		weights[i].weight.x = pmd->vertices[i].weight.b_weight[0] / 100.0f;
		weights[i].weight.y = pmd->vertices[i].weight.b_weight[1] / 100.0f;
		weights[i].weight.z = 0;
		weights[i].weight.w = 0;
		weights[i].idx.x    = pmd->vertices[i].weight.b_id[0];
		weights[i].idx.y    = pmd->vertices[i].weight.b_id[1];
		weights[i].idx.z    = -1;
		weights[i].idx.w    = -1;
	}

	*w = weights;
}

void PMDImporter::ImportModel(String path, R3DStaticModelInfo* info) {
	pmd_file*         pmd;
	R3D_Vertex*       vertices;
	Uint16*           indices;
	R3D_MaterialInfo* materialInfo;
	R3D_MatMeshInfo*  meshInfo;

	LoadPMD(path, &pmd, &vertices, &indices);
	LoadPMDMaterials(pmd, &materialInfo , &meshInfo);


	// Materials
	info->matInfo  = materialInfo;
	info->matCount = pmd->material_count;

	// Meshes
	info->mInfo    = meshInfo;
	info->mCount   = pmd->material_count;

	// Data
	info->vertices = vertices;
	info->vCount   = pmd->vertex_count;
	info->indices  = indices;
	info->iCount   = pmd->index_count;
	info->iType    = RG_INDEX_U16;

	pmd_free(pmd);

}

void PMDImporter::FreeModelData(R3DStaticModelInfo* info) {
	rg_free(info->vertices);
	rg_free(info->indices);
	rg_free(info->matInfo);
	rg_free(info->mInfo);
}

void PMDImporter::ImportRiggedModel(String path, R3DRiggedModelInfo* info) {
	pmd_file*         pmd;
	R3D_Vertex*       vertices;
	R3D_Weight*       weights;
	Uint16*           indices;
	R3D_MaterialInfo* materialInfo;
	R3D_MatMeshInfo*  meshInfo;

	LoadPMD(path, &pmd, &vertices, &indices);
	LoadPMDMaterials(pmd, &materialInfo, &meshInfo);
	LoadPMDWeights(pmd, &weights);

	// Materials
	info->matInfo  = materialInfo;
	info->matCount = pmd->material_count;

	// Meshes
	info->mInfo    = meshInfo;
	info->mCount   = pmd->material_count;

	// Data
	info->vertices = vertices;
	info->weights  = weights;
	info->vCount   = pmd->vertex_count;
	info->indices  = indices;
	info->iCount   = pmd->index_count;
	info->iType    = RG_INDEX_U16;

	pmd_free(pmd);

}

void PMDImporter::FreeRiggedModelData(R3DRiggedModelInfo* info) {
	rg_free(info->vertices);
	rg_free(info->indices);
	rg_free(info->weights);
	rg_free(info->matInfo);
	rg_free(info->mInfo);
}

// TODO: Rewrite this
KinematicsModel* PMDImporter::ImportKinematicsModel(String file) {
	pmd_file* pmd = pmd_load(file);

	BoneInfo bones_info[1024];
	mat4 bone_matrices[1024];

	const char CONSTNAME[] = { 0xE3, 0x81, 0xB2, 0xE3, 0x81, 0x96, 0x00 };//"ひざ";

	for (Uint32 i = 0; i < pmd->bones_count; i++) {
		pmd_bone* bone = &pmd->bones[i];
		mat4 parent = MAT4_IDENTITY();
		vec3 pos = bone->position;
		if (bone->parent != -1) {
			pmd_bone* pbone = &pmd->bones[bone->parent];
			pos = bone->position - pbone->position;
			parent = bone_matrices[bone->parent];
		}

		bones_info[i].offset_pos = pos;


		quat rot = { 0, 0, 0, 1 };
		mat4 translation;
		mat4 rotation;
		mat4_fromquat(&rotation, rot);
		mat4_translate(&translation, pos);

		mat4 local = translation * rotation;
		bone_matrices[i] = parent * local;
		mat4_inverse(&bones_info[i].offset, bone_matrices[i]);

		bones_info[i].parent = bone->parent;
		SDL_strlcpy(bones_info[i].name, bone->name, 32);

		bones_info[i].has_limit = false;
		if (strstr(bones_info[i].name, CONSTNAME) != NULL) {
			bones_info[i].has_limit = true;
			bones_info[i].limitation = { 1, 0, 0 };
		}

		//Uint32 hash = rgCRC32(bones_info[i].name, SDL_strlen(bones_info[i].name));
		//rgLogWarn(RG_LOG_GAME, "Bone: %d p: %d, name: %s - %d", i, bones_info[i].parent, bones_info[i].name, hash);
		//PRINTMATRIX(bone_matrices[i]);
		//PRINTMATRIX(bones_info[i].offset);
	}

	Uint32 ik = pmd->ik_count;
	IKList* ik_links = (IKList*)rg_malloc(sizeof(IKList) * ik);
	for (Uint32 i = 0; i < ik; i++) {
		pmd_ik_info* ik_info = &pmd->ik[i];
		ik_links[i].angle_limit = ik_info->angle_limit;
		ik_links[i].bones       = ik_info->bones;
		ik_links[i].effector    = ik_info->effector;
		ik_links[i].target      = ik_info->target;
		ik_links[i].iterations  = ik_info->max_iterations;
		for (Uint32 j = 0; j < ik_info->bones; j++) {
			ik_links[i].list[j] = ik_info->list[j];
		}
	}

	// Bone buffer
	R3DCreateBoneBufferInfo binfo = {};
	binfo.len         = sizeof(mat4) * pmd->bones_count;
	binfo.initialData = NULL;
	R3D_BoneBuffer* bone_buffer = Render::CreateBoneBuffer(&binfo);

	// Kinematics model
	KinematicsModelCreateInfo info = {};
	info.bone_count    = pmd->bones_count;
	info.bones_info    = bones_info;
	info.ik_count      = ik;
	info.ik_info       = ik_links;
	info.buffer_handle = bone_buffer;
	KinematicsModel* kmodel = RG_NEW_CLASS(GetDefaultAllocator(), KinematicsModel)(&info);
	//KinematicsModel* kmodel = new KinematicsModel(&info);

	rg_free(ik_links);
	pmd_free(pmd);

	return kmodel;
}

Animation* VMDImporter::ImportAnimation(String path, KinematicsModel* model) {

	vmd_file* vmd = vmd_load(path);

	Animation* animation = new Animation();
	Uint32 last = 0;
	for (Uint32 i = 0; i < vmd->motion_count; i++) {
		vmd_motion* motion = &vmd->motions[i];
		Uint32 hash = rgCRC32(motion->bone_name, SDL_strlen(motion->bone_name));

		// Do not add animation track if in model "motion->bone_name" bone not exist.
		if (model != NULL && model->GetBoneByCRCHash(hash) == NULL) {
			//rgLogWarn(RG_LOG_SYSTEM, "Skipping: %s - CRC: %d", motion->bone_name, hash);
			continue;
		}

		AnimationTrack* track = animation->GetBoneAnimationTrack(hash);
		if (track == NULL) {
			track = new AnimationTrack(motion->bone_name);
			animation->AddBoneAnimationTrack(track);
			//rgLogInfo(RG_LOG_SYSTEM, "Adding: %s - CRC: %d", motion->bone_name, hash);
		}

		BoneKeyFrame keyframe = {};
		keyframe.scale.x     = 1;
		keyframe.scale.y     = 1;
		keyframe.scale.z     = 1;
		keyframe.translation = motion->position;
		keyframe.rotation    = motion->rotation;
		keyframe.timestamp   = motion->frame;

		keyframe.interp_x.x = motion->interpolation[0]  / 127.0f;
		keyframe.interp_x.y = motion->interpolation[4]  / 127.0f;
		keyframe.interp_x.z = motion->interpolation[8]  / 127.0f;
		keyframe.interp_x.w = motion->interpolation[12] / 127.0f;
		keyframe.interp_y.x = motion->interpolation[16] / 127.0f;
		keyframe.interp_y.y = motion->interpolation[20] / 127.0f;
		keyframe.interp_y.z = motion->interpolation[24] / 127.0f;
		keyframe.interp_y.w = motion->interpolation[28] / 127.0f;
		keyframe.interp_z.x = motion->interpolation[32] / 127.0f;
		keyframe.interp_z.y = motion->interpolation[36] / 127.0f;
		keyframe.interp_z.z = motion->interpolation[40] / 127.0f;
		keyframe.interp_z.w = motion->interpolation[44] / 127.0f;
		keyframe.interp_r.x = motion->interpolation[48] / 127.0f;
		keyframe.interp_r.y = motion->interpolation[52] / 127.0f;
		keyframe.interp_r.z = motion->interpolation[56] / 127.0f;
		keyframe.interp_r.w = motion->interpolation[60] / 127.0f;

		track->AddKeyFrame(&keyframe);
		if (last < motion->frame) {
			last = motion->frame;
		}
	}
	animation->Finish(last);

	return animation;

}
