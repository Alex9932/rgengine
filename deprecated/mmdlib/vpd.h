/*
 * vpd.h
 *
 *  Created on: Apr 1, 2022
 *      Author: alex9932
 *
 *  VPD (Vocaloid Pose Data) Implementation
 *  Spec:
 *  	-
 */

#ifndef VPD_H_
#define VPD_H_

#include <rgvector.h>

typedef struct vpd_bone {
	char bone_name[32];
	Uint32 id;
	vec3 position;
	vec4 rotation;
} vpd_bone;

typedef struct vpd_pose {
	Uint32 bone_count;
	vpd_bone* bones;
} vpd_pose;

RG_DECLSPEC vpd_pose* vpd_fromFile(const char* file);
RG_DECLSPEC void vpd_free(vpd_pose* ptr);

#endif /* VPD_H_ */
