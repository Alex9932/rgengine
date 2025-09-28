/*
 * pmd.h
 *
 *  Created on: Mar 20, 2022
 *      Author: alex9932
 *
 *  PMD (Polygon Model Data) Implementation
 *  Spec:
 *  	https://mikumikudance.fandom.com/wiki/MMD:Polygon_Model_Data
 */


#ifndef PMD_H_
#define PMD_H_

//#define STD_PMD

#include <SDL3/SDL.h>
#include <rgvector.h>

#define PMD_MAX_BONES 1024
#define PMD_MAX_FACES 1024

#define PMD_RIGIDBODY_TYPE_KINEMATIC 0
#define PMD_RIGIDBODY_TYPE_SIMULATED 1
#define PMD_RIGIDBODY_TYPE_ALIGNED   2

typedef struct pmd_weight {
	Uint16 b_id[2];
	Uint16 b_weight[2];
} pmd_weight;

typedef struct pmd_position {
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	float u;
	float v;
} pmd_position;

typedef struct pmd_material_colors {
	float r;
	float g;
	float b;
	float a;
	float shininess;
	float specular_r;
	float specular_g;
	float specular_b;
	float ambient_r;
	float ambient_g;
	float ambient_b;
} pmd_material_colors;

typedef struct pmd_material {
	pmd_material_colors colors;
	Uint16 toon_number;
	Uint16 edge_flag;
	Uint32 surface_count;
	char file_name[20];
} pmd_material;

typedef struct pmd_vertex {
	pmd_position vertex;
	pmd_weight   weight;
	Uint32       edge;
} pmd_vertex;

typedef struct pmd_bone {
	char name[32];
	Sint16 parent;
	Sint16 child;
	Uint16 bone_type;
	Uint16 bone_target;
	vec3 position;
} pmd_bone;

typedef struct pmd_ik_info {
	Uint16 target;
	Uint16 effector;
	Uint16 bones;
	Uint16 max_iterations;
	float angle_limit;
	Uint16* list;
} pmd_ik_info;

typedef struct pmd_morph_vertex {
	Uint32 vertex_id;
	float x;
	float y;
	float z;
} pmd_morph_vertex;

typedef struct pmd_morph {
	char name[32];
	Uint32 vertices;
	Uint32 face_type;
	pmd_morph_vertex* list;
} pmd_morph;

// Extended format structures
typedef struct pmd_rigidbody {
	char name[20];
	Uint16 bone_id;
	Uint16 collision_group; // Uint8 in file
	Uint16 collision_group_mask;
	Uint16 collision_shape; // Uint8 in file
	float w;
	float h; // Radius (if sphere shape)
	float d; // Valid only for box shape
	float x; // XYZ position (relative to the bone it is attached to)
	float y;
	float z;
	float rx; // Rotation
	float ry;
	float rz;
	float mass;
	float linear_dampening;
	float angular_dampening;
	float restitution_coefficient;
	float friction_coefficient;
	Uint32 body_type; // Uint8 in file
} pmd_rigidbody;

typedef struct pmd_constraint {
	char name[20];
	Uint32 first_body;
	Uint32 second_body;
	float x; // Relative to first rigid body
	float y;
	float z;
	float rx; // Rotation
	float ry;
	float rz;
	float x_lower_limit; // Limits
	float y_lower_limit;
	float z_lower_limit;
	float x_upper_limit;
	float y_upper_limit;
	float z_upper_limit;
	float rx_lower_limit;
	float ry_lower_limit;
	float rz_lower_limit;
	float rx_upper_limit;
	float ry_upper_limit;
	float rz_upper_limit;
	float x_stiffness;
	float y_stiffness;
	float z_stiffness;
	float rx_stiffness;
	float ry_stiffness;
	float rz_stiffness;
} pmd_constraint;

typedef struct pmd_file {
	char path[256];
	char signature[3];
	Uint8 padding;
	float version;
	char name[20];
	char comment[256];
	Uint32 vertex_count;
	Uint32 index_count;
	Uint32 material_count;
	Uint32 bones_count;
	Uint32 ik_count;
	Uint32 morph_count;
	pmd_vertex* vertices;
	Uint16* indices;
	pmd_material* materials;
	pmd_bone* bones;
	pmd_ik_info* ik;
	pmd_morph* morphs;
	// END OF THE BASE MODEL FORMAT

	bool is_extended;
	Uint32 bone_groups;
	char e_name[20];
	char e_comment[256];
	char** e_bones;
	char** e_faces;
	char** e_bone_group;
	char** e_toon_textures;
	Uint32 rigidbody_count;
	Uint32 constraint_count;
	pmd_rigidbody* rigidbodies;
	pmd_constraint* constraints;

} pmd_file;

RG_DECLSPEC pmd_file* pmd_load(const char* file);
RG_DECLSPEC void pmd_free(pmd_file* ptr);

#endif /* PMD_H_ */
