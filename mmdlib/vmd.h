/*
 * vmd.h
 *
 *  Created on: Mar 19, 2022
 *      Author: alex9932
 *
 *  VMD (Vocaloid Motion Data) Implementation
 *  Spec:
 *  	https://mikumikudance.fandom.com/wiki/VMD_file_format
 */

#ifndef VMD_H_
#define VMD_H_

#include <rgvector.h>

typedef struct vmd_motion {
	char bone_name[32];
	Uint32 frame;
	vec3 position;
	quat rotation;
	Uint8 interpolation[64];
} vmd_motion;

typedef struct vmd_face {
	char name[32];
	Uint32 frame;
	float weight;
} vmd_face;

typedef struct vmd_camera {
	Uint32 frame;
	float distance;
	float target_x;
	float target_y;
	float target_z;
	float camera_rx;
	float camera_ry;
	float camera_rz;
	char interpolation[24];
	Uint32 camera_fov;
	Uint32 camera_perspective;
} vmd_camera;

typedef struct vmd_file {
	char magic[30]; // "Vocaloid Motion Data 0002\0\0\0\0\0"
	char name[20];
	Sint32 motion_count;
	Sint32 face_count;
	Sint32 camera_count;
	vmd_motion* motions;
	vmd_face* faces;
	vmd_camera* cameras;
} vmd_file;

RG_DECLSPEC vmd_file* vmd_load(String file);
RG_DECLSPEC void vmd_free(vmd_file* ptr);

#endif /* VMD_H_ */
