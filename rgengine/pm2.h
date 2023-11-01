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

#define PM2_FLAG_IS_RIGGED        0b00000001 // Additional information (Array of vertex weights and bone IDs, skeleton information)
#define PM2_FLAG_EXTENDED_INDICES 0b00000010 // Use 32-bit indices (In pm2 format 16-bit indices used by default)

typedef struct PM2_Header {
	char sig[4];
	Uint32 materials;
	Uint32 vertices;
	Uint32 indices;
	Uint8 flags;
	Uint8 version;
	Uint16 offset; // Mesh count in PM2 v2
} PM2_Header;

typedef vec4 PM2_Color;
typedef vec3 PM2_Vec3;
typedef vec2 PM2_Vec2;

typedef struct PM2_String {
	Uint32 len;
	char* str;
} PM2_String;

typedef struct PM2_V1_Material {
	PM2_String name;
	Uint32 indices;
	PM2_Color diffuse;
	PM2_Color ambient;
} PM2_V1_Material;

typedef struct PM2_V2_Material {
	PM2_String albedo;
	PM2_String normal;
	PM2_Color color;
} PM2_V2_Material;

typedef struct PM2_V2_MeshInfo {
	Uint32 material;
	Uint32 indices;
} PM2_V2_MeshInfo;

typedef struct PM2_Vertex {
	PM2_Vec3 position;
	PM2_Vec3 normal;
	PM2_Vec3 tangent;
	PM2_Vec2 uv;
} PM2_Vertex;

#if 0
RG_DECLSPEC R3D_StaticModel* PM2_LoadModel(String path);
#endif

#endif