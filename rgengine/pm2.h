/*
 * rgEngine core/pm2.h
 *
 *  Created on: Jan 6, 2023
 *      Author: alex9932
 * 
 *  PM2 File format structure difinitions
 * 
 */

#ifndef CORE_PM2_H
#define CORE_PM2_H

#include "rgtypes.h"
#include "rgmath.h"
#include "rendertypes.h"

#define PM2_FLAG_SKELETON         0b00000001 // Additional information (Array of vertex weights and bone IDs, skeleton information)
#define PM2_FLAG_EXTENDED_INDICES 0b00000010 // Use 32-bit indices (In pm2 format 16-bit indices used by default)

typedef struct PM2_Header {
	char   sig[4];
	Uint32 materials;
	Uint32 vertices;
	Uint32 indices;
	Uint8  flags;
	Uint8  version;
	Uint16 offset; // Mesh count in PM2 version >=2
} PM2_Header;

typedef struct PM2_SkeletonHeader {
	char   sig[4];
	Uint32 bones;
	Uint32 ikchains;
} PM2_SkeletonHeader;

typedef struct PM2_String {
	Uint32 len;
	char* str;
} PM2_String;

// Deprecated formats
typedef struct PM2_V1_Material {
	PM2_String name;
	Uint32     indices;
	vec4       diffuse;
	vec4       ambient;
} PM2_V1_Material;

typedef struct PM2_V2_Material {
	PM2_String albedo;
	PM2_String normal;
	vec4       color;
} PM2_V2_Material;

typedef struct PM2_V3_Material {
	PM2_String albedo;
	PM2_String normal;
	PM2_String pbr;
	vec4       color;
} PM2_V3_Material;

// Current format
// Work in progress
typedef struct PM2_V4_Material {
	// Albedo - %gamedata%/textures/@texture@.png
	// Normal - %gamedata%/textures/@texture@_norm.png
	// PBR    - %gamedata%/textures/@texture@_pbr.png
	PM2_String texture;
	vec4       color;
} PM2_V4_Material;

typedef struct PM2_MeshInfo {
	Uint32 material;
	Uint32 indices;
} PM2_MeshInfo;

typedef struct PM2_Vertex {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
} PM2_Vertex;

typedef struct PM2_Weight {
	vec4  weights;
	ivec4 boneids;
} PM2_Weight;

typedef struct PM2_Bone {
	PM2_String name;
	Sint16     parent;
	vec3       position;
} PM2_Bone;

typedef struct PM2_IKChain {
	Uint16  target;
	Uint16  effector;
	Uint16  iterations;
	Float32 angle_limit;
	Uint16  bones;
	Uint16* list;
} PM2_IKChain;

#endif