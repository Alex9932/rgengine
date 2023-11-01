/*
 * vmd.cpp
 *
 *  Created on: Mar 19, 2022
 *      Author: alex9932
 */

#define _CRT_SECURE_NO_WARNINGS

#include "vmd.h"

#include <map>
#include <utf8.h>
#include <engine.h>
#include <rgstring.h>
#include <filesystem.h>
#include <allocator.h>

#if 0

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

PACK (struct _vmd_motion {
	char bone_name[15];
	Uint32 frame;
	vec3 position;
	quat rotation;
	Uint8 interpolation[64];
});

#endif

vmd_file* vmd_load(String file) {

	Engine::FSReader* reader = new Engine::FSReader(file);
	rgLogInfo(RG_LOG_SYSTEM, "VMD: Loading animation: %s\n", file);

	vmd_file* vmd = (vmd_file*)rg_malloc(sizeof(vmd_file));

	reader->Read(vmd->magic, 30);

	if(!Engine::rg_strstw(vmd->magic, "Vocaloid Motion Data")) {
		char buffer[128];
		SDL_snprintf(buffer, 128, "%s is not a Vocaloid Motion Data file!", file);
		RG_ERROR_MSG(buffer);
	}

	reader->Read(vmd->name, 20);

	vmd->motion_count = reader->ReadU32();
	vmd->motions = (vmd_motion*)rg_malloc(sizeof(vmd_motion) * vmd->motion_count);
	//_vmd_motion motion;

	// TODO !!! REWRITE THIS !!!

	char name_buffer[32];
	for (Sint32 i = 0; i < vmd->motion_count; ++i) {

		vmd_motion* mot = &vmd->motions[i];

		reader->Read(name_buffer, 15);
		UTF8_FromSJIS(name_buffer);

		SDL_memset(mot->bone_name, 0, 32);
		SDL_memcpy(mot->bone_name, UTF8_GetBuffer(), min(SDL_strlen(UTF8_GetBuffer()), 31));

		//rgLogWarn(RG_LOG_RENDER, "VMD MOTION: %d %s (%d)", i, mot->bone_name, SDL_strlen(mot->bone_name));
		

		//reader->Read(mot->bone_name, 15);
		mot->frame = reader->ReadU32();
#if 0
		mot->position.x = -reader->ReadF32();
		mot->position.y = reader->ReadF32();
		mot->position.z = reader->ReadF32();
		mot->rotation.x = reader->ReadF32();
		mot->rotation.y = -reader->ReadF32();
		mot->rotation.z = -reader->ReadF32();
		mot->rotation.w = reader->ReadF32();
#else
		mot->position.x = reader->ReadF32();
		mot->position.y = reader->ReadF32();
		mot->position.z = reader->ReadF32();
		mot->rotation.x = -reader->ReadF32();
		mot->rotation.y = -reader->ReadF32();
		mot->rotation.z = -reader->ReadF32();
		mot->rotation.w = -reader->ReadF32();
#endif
		reader->Read(mot->interpolation, 64);

//		reader->Read(name_buffer, 15);
//		MMD_Utils::SJISToUTF8(name_buffer);
//		memcpy(vmd->motions[i].bone_name, MMD_Utils::GetBuffer(), strlen(MMD_Utils::GetBuffer()));
//
//		vmd->motions[i].frame = reader->ReadU32();
//		reader->Read(&vmd->motions[i].position, sizeof(pmx_vec3));
//		reader->Read(&vmd->motions[i].rotation, sizeof(pmx_vec4));

//		vmd->motions[i].position.x = reader->ReadF32();
//		vmd->motions[i].position.y = reader->ReadF32();
//		vmd->motions[i].position.z = reader->ReadF32();
//		vmd->motions[i].rotation.x = reader->ReadF32();
//		vmd->motions[i].rotation.y = reader->ReadF32();
//		vmd->motions[i].rotation.z = reader->ReadF32();
//		vmd->motions[i].rotation.w = reader->ReadF32();

//		reader->Read3F32(vmd->motions[i].position);
//		reader->Read4F32(vmd->motions[i].rotation);
//		reader->Read(vmd->motions[i].interpolation, 64);

#if 0
		reader->Read(&motion, sizeof(_vmd_motion));
		SDL_memset(vmd->motions[i].bone_name, 0, 32);
		UTF8_FromSJIS(motion.bone_name);
		SDL_memcpy(vmd->motions[i].bone_name, UTF8_GetBuffer(), strlen(UTF8_GetBuffer()));
		vmd->motions[i].frame = motion.frame;
		vmd->motions[i].position = motion.position;
		vmd->motions[i].rotation = motion.rotation;
//		vmd->motions[i].position.x = -motion.position.x;
//		vmd->motions[i].position.y = motion.position.y;
//		vmd->motions[i].position.z = motion.position.z;
//		vmd->motions[i].rotation.x = motion.rotation.x;
//		vmd->motions[i].rotation.y = -motion.rotation.y;
//		vmd->motions[i].rotation.z = -motion.rotation.z;
//		vmd->motions[i].rotation.w = motion.rotation.w;
		SDL_memcpy(vmd->motions[i].interpolation, motion.interpolation, 64);

//		if(i == 3) {
//			rgLogWarn(RG_LOG_SYSTEM, "frame: %d, pos: %f %f %f, rot: %f %f %f %f",
//					vmd->motions[i].frame,
//					vmd->motions[i].position.x,
//					vmd->motions[i].position.y,
//					vmd->motions[i].position.z,
//					vmd->motions[i].rotation.x,
//					vmd->motions[i].rotation.y,
//					vmd->motions[i].rotation.z,
//					vmd->motions[i].rotation.w);
//		}
#endif
	}

	vmd->face_count = reader->ReadU32();
	vmd->faces = (vmd_face*)rg_malloc(sizeof(vmd_face) * vmd->face_count);
	char char_buffer[15];
	for (Sint32 i = 0; i < vmd->face_count; ++i) {
		reader->Read(&char_buffer, 15);
		SDL_memset(vmd->faces[i].name, 0, 32);
		UTF8_FromSJIS(char_buffer);
		SDL_memcpy(vmd->faces[i].name, UTF8_GetBuffer(), strlen(UTF8_GetBuffer()));
		vmd->faces[i].frame = reader->ReadU32();
		vmd->faces[i].weight = reader->ReadF32();
	}

	vmd->camera_count = reader->ReadU32();
	vmd->cameras = (vmd_camera*)rg_malloc(sizeof(vmd_camera) * vmd->camera_count);
	for (Sint32 i = 0; i < vmd->camera_count; ++i) {
		vmd->cameras[i].frame = reader->ReadU32();
		vmd->cameras[i].distance = reader->ReadF32();
		vmd->cameras[i].target_x = reader->ReadF32();
		vmd->cameras[i].target_y = reader->ReadF32();
		vmd->cameras[i].target_z = reader->ReadF32();
		vmd->cameras[i].camera_rx = reader->ReadF32();
		vmd->cameras[i].camera_ry = reader->ReadF32();
		vmd->cameras[i].camera_rz = reader->ReadF32();
		reader->Read(vmd->cameras[i].interpolation, 24);
		vmd->cameras[i].camera_fov = reader->ReadU32();
		vmd->cameras[i].camera_perspective = reader->ReadU8();
	}

	delete reader;
	return vmd;
}

void vmd_free(vmd_file* ptr) {
	rg_free(ptr->motions);
	rg_free(ptr->faces);
	rg_free(ptr->cameras);
	rg_free(ptr);
}
