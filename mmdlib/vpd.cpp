/*
 * vpd.cpp
 *
 *  Created on: Apr 1, 2022
 *      Author: alex9932
 */

#define DLL_EXPORT
#define _CRT_SECURE_NO_WARNINGS

#include "vpd.h"
#include <vector>
#include <SDL2/SDL.h>

#define LINE_LENGTH 128

#include <utf8.h>
#include <rgstring.h>
#define streql Engine::rg_streql
#define strstw Engine::rg_strstw

//static bool streql(const char* left, const char* right) {
//	size_t lenpre = strlen(left);
//	size_t lenstr = strlen(right);
//	return ( (lenstr == lenpre) && (SDL_memcmp(left, right, lenpre) == 0) ) ? true : false;
//}
//
//// Starts with
//static bool strstw(const char* left, const char* right) {
//	size_t lenstr = strlen(right);
//	for (Uint32 i = 0; i < lenstr; ++i) {
//		if(left[i] != right[i]) {
//			return false;
//		}
//	}
//	return true;
//}

static char LINE_BUFFER[LINE_LENGTH];
static bool readLine(FILE* f) {
	SDL_memset(LINE_BUFFER, 0, LINE_LENGTH);
	return fgets(LINE_BUFFER, LINE_LENGTH, f);
}

static void COPY_STRING(char* dst, const char* src, size_t len) {
//	printf("String: ");
	for (size_t i = 0; i < len; ++i) {
//		printf("%c (%x) ", src[i], src[i]);
		if(src[i] == '\n' || src[i] == '\r') {
			dst[i] = '\0';
			break;
		} else {
			dst[i] = src[i];
		}
	}
//	printf("\n");
}

static Sint32 getBoneID(char* name) {
	char* ptr = strchr(LINE_BUFFER, '{');
	if(ptr == NULL) {
		return -1;
	}

	// index

	int size = ptr - &LINE_BUFFER[4];
	char buffer[8];
	memset(buffer, 0, 8);
	memcpy(buffer, &LINE_BUFFER[4], size);
	Sint32 index = atoi(buffer);

	// name

	char* sjis_name = &ptr[1];
	UTF8_FromSJIS(sjis_name);

//	printf("~vpd name: %s\n", name);

	COPY_STRING(name, UTF8_GetBuffer(), SDL_strlen(UTF8_GetBuffer()));
//	SDL_memcpy(name, MMD_Utils::GetBuffer(), SDL_strlen(MMD_Utils::GetBuffer()));


//	printf("vpd name %d: %s\n", index, name);

	return index;
}

static Sint32 getStart() {
	for (Uint32 i = 0; i < LINE_LENGTH; ++i) {
		char c = LINE_BUFFER[i];
		if(c == '-' || c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
			c == '5' || c == '6' || c == '7' || c == '8' || c == '9') {
			return i;
		}
	}
	return -1;
}

static Sint32 getEnd() {
	char* ptr = strchr(LINE_BUFFER, ';');
	if(ptr == NULL) {
		return -1;
	}

	return ptr - LINE_BUFFER;
}

static void getVector(char* dest) {
	Sint32 start = getStart();
	Sint32 end = getEnd();
	int size = end - start;
	SDL_memcpy(dest, &LINE_BUFFER[start], size);
}

static vec3 parsePosition() {
	char buffer[LINE_LENGTH];
	SDL_memset(buffer, 0, LINE_LENGTH);
	getVector(buffer);

	const Uint32 n = 3;
	char pos_buff[n][16];
	char* ptr = buffer;
	char* ptr1;
	for (Uint32 i = 0; i < n; ++i) {
		SDL_memset(pos_buff[i], 0, 16);
		if(i == n - 1) { ptr1 = (char*)(ptr + strlen(ptr)); }
		else { ptr1 = strchr(ptr, ','); }
		SDL_memcpy(pos_buff[i], ptr, ptr1 - ptr);
		ptr = ptr1 + 1;
	}

//	printf("VPD: Pos: %s {%s %s %s}\n", buffer, pos_buff[0], pos_buff[1], pos_buff[2]);

	vec3 pos;
	pos.x = atof(pos_buff[0]);
	pos.y = atof(pos_buff[1]);
	pos.z = atof(pos_buff[2]);
	return pos;
}

static vec4 parseRotation() {
	char buffer[LINE_LENGTH];
	SDL_memset(buffer, 0, LINE_LENGTH);
	getVector(buffer);

	const Uint32 n = 4;
	char pos_buff[n][16];
	char* ptr = buffer;
	char* ptr1;
	for (Uint32 i = 0; i < n; ++i) {
		SDL_memset(pos_buff[i], 0, 16);
		if(i == n - 1) { ptr1 = (char*)(ptr + strlen(ptr)); }
		else { ptr1 = strchr(ptr, ','); }
		SDL_memcpy(pos_buff[i], ptr, ptr1 - ptr);
		ptr = ptr1 + 1;
	}

//	printf("VPD: Rot: %s\n", buffer);
	vec4 quat;

	quat.x = atof(pos_buff[0]);
	quat.y = atof(pos_buff[1]);
	quat.z = atof(pos_buff[2]);
	quat.w = atof(pos_buff[3]);

	return quat;
}

vpd_pose* vpd_fromFile(const char* file) {
	rgLogInfo(RG_LOG_SYSTEM, "VPD: Loading pose: %s", file);
	FILE* f = fopen(file, "r");
	if(!f) {
		return NULL;
	}

	// Read data
	char vpd[19];
	SDL_memset(vpd, 0, 19);
	fread(vpd, 18, 1, f);
	if(!streql(vpd, "Vocaloid Pose Data")) {
		rgLogError(RG_LOG_SYSTEM, "VPD: %s is not Vocaloid Pose Data file!", file);
		fclose(f);
		return NULL;
	}

	std::vector<vpd_bone> bones;
	Sint32 bone = -1;
	char b_name[32];
	Uint32 line = 0;
	bool pos = false;
	vec3 position;
	vec4 rotation;
	while(readLine(f)) {
		line++;

		if(LINE_BUFFER[0] != '\n') {
			if(strstw(LINE_BUFFER, "Bone")) {
				SDL_memset(b_name, 0, 32);
				bone = getBoneID(b_name);
				if(bone == -1) {
					rgLogError(RG_LOG_SYSTEM, "VPD: Parse error! Line[%d]: %s", line, LINE_BUFFER);
					return NULL;
				}
//				printf("VPD: %s", LINE_BUFFER);
			} else if(strstw(LINE_BUFFER, "}")) {
				if(bone == -1) {
					rgLogError(RG_LOG_SYSTEM, "VPD: Parse error! Line[%d]: %s", line, LINE_BUFFER);
					return NULL;
				}

				vpd_bone b;
				b.id = bone;
				SDL_memcpy(b.bone_name, b_name, 32);
				b.position = position;
				b.rotation = rotation;
				bones.push_back(b);
				bone = -1;
			} else {
				if(bone != -1) {
					if(!pos) {
						position = parsePosition();
						pos = true;
					} else {
						rotation = parseRotation();
						pos = false;
					}
				}
			}
		}
	}

	vpd_pose* pose = (vpd_pose*)malloc(sizeof(vpd_pose));
	pose->bone_count = bones.size();
	pose->bones = (vpd_bone*)malloc(sizeof(vpd_bone) * pose->bone_count);
	for (Uint32 i = 0; i < pose->bone_count; ++i) {
		pose->bones[i] = bones[i];
	}
	fclose(f);
	return pose;
}

void vpd_free(vpd_pose* ptr) {
	free(ptr->bones);
	free(ptr);
}
