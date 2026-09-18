/*
 * pmx.cpp
 *
 *  Created on: Jan 27, 2022
 *      Author: alex9932
 */

#define DLL_EXPORT

#include "pmx.h"
#include <allocator.h>
#include <filesystem.h>

#include <utf8.h>

////////////////////////////////////////
// Rewrite this

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#endif

PACK(struct _pmx_header {
	Uint8 signature[4];            // (PMX [0x50, 0x4D, 0x58, 0x20])
	float version;                 // 2.0 / 2.1
	Uint8 globals_count;           // For pmx v2.0 fixed at 8
	// This implementation supports only 8 globals
	Uint8 g_text_encoding;         // Text encoding 0 = UTF16LE, 1 = UTF8
	Uint8 g_additional_vec4_count; // Additional vec4 values are added to each vertex (0 .. 4)
	Uint8 g_vertex_index_size;     // Index types {1, 2 or 4}
	Uint8 g_texture_index_size;
	Uint8 g_material_index_size;
	Uint8 g_bone_index_size;
	Uint8 g_morph_index_size;
	Uint8 g_rigidbody_index_size;
});

PACK(struct _pmx_vertex {
	vec3 position;
	vec3 normal;
	vec2 uv;
});

#define PMX_DEBUG 1

////////////////////////////////////////

static Sint32 _pmx_readSINT(Uint8 size, Engine::FSReader* reader) {
	switch (size) {
	case 1: { Sint8 n = reader->ReadS8(); return n; }
	case 2: { Sint16 n = reader->ReadS16(); return n; }
	case 4: { Sint32 n = reader->ReadS32(); return n; }
	}
	return -1;
}

static Uint32 _pmx_readUINT(Uint8 size, Engine::FSReader* reader) {
	switch (size) {
	case 1: { return reader->ReadU8();  }
	case 2: { return reader->ReadU16(); }
	case 4: { return reader->ReadU32(); }
	}
	return -1;
}

static void _pmx_readVertex(Sint32 id, pmx_file* pmx, Engine::FSReader* reader, bool* bSDEF) {
	pmx_vertex vertex;
	SDL_memset(&vertex, 0, sizeof(pmx_vertex));

	_pmx_vertex vert;
	reader->Read(&vert, sizeof(_pmx_vertex));
	vertex.position = vert.position;
	vertex.normal = vert.normal;
	vertex.uv = vert.uv;

	// Additional vec4
	vertex.additional_vec4 = (vec4*)rg_malloc(sizeof(vec4) * pmx->header.g_additional_vec4_count);
	reader->Read(vertex.additional_vec4, sizeof(vec4) * pmx->header.g_additional_vec4_count);

	vertex.type = reader->ReadU8();

	// Weights
	switch (vertex.type) {
	case 0: // BDEF 1
		vertex.weight.bone_id[0] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[1] = -1;
		vertex.weight.bone_id[2] = -1;
		vertex.weight.bone_id[3] = -1;
		vertex.weight.weights[0] = 1;
		vertex.weight.weights[1] = 0;
		vertex.weight.weights[2] = 0;
		vertex.weight.weights[3] = 0;
		break;
	case 1: // BDEF 2
		vertex.weight.bone_id[0] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[1] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[2] = -1;
		vertex.weight.bone_id[3] = -1;
		vertex.weight.weights[0] = reader->ReadF32();
		vertex.weight.weights[1] = 1.0f - vertex.weight.weights[0];
		vertex.weight.weights[2] = 0;
		vertex.weight.weights[3] = 0;
		break;
	case 2: // BDEF 4
		vertex.weight.bone_id[0] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[1] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[2] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[3] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.weights[0] = reader->ReadF32();
		vertex.weight.weights[1] = reader->ReadF32();
		vertex.weight.weights[2] = reader->ReadF32();
		vertex.weight.weights[3] = reader->ReadF32();
		break;
	case 3: // SDEF
		vertex.weight.bone_id[0] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[1] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[2] = -1;
		vertex.weight.bone_id[3] = -1;
		vertex.weight.weights[0] = reader->ReadF32();
		vertex.weight.weights[1] = 1.0f - vertex.weight.weights[0];
		vertex.weight.weights[2] = 0;
		vertex.weight.weights[3] = 0;
		vec3 C, R0, R1;
		reader->Read3F32(C);  // Not supported! Just read 3 vec3
		reader->Read3F32(R0);
		reader->Read3F32(R1);
		if (!(*bSDEF)) {
			rgLogWarn(RG_LOG_SYSTEM, "PMX: Model cantains not fully implemented SDEF vertices, this may cause glitches!");
			*bSDEF = true;
		}
		break;
	case 4: // QDEF
		vertex.weight.bone_id[0] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[1] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[2] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.bone_id[3] = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		vertex.weight.weights[0] = reader->ReadF32();
		vertex.weight.weights[1] = reader->ReadF32();
		vertex.weight.weights[2] = reader->ReadF32();
		vertex.weight.weights[3] = reader->ReadF32();
		break;
	default:
		break;
	}

	vertex.edge_scale = reader->ReadF32();
	pmx->vertices[id] = vertex;
}

static pmx_text _pmx_readText(Engine::FSReader* reader) {
	pmx_text text;
	text.len = reader->ReadU32();
	text.data = (char*)rg_malloc(text.len);
	reader->Read(text.data, text.len);
	return text;
}

static void _pmx_readTexture(Sint32 id, pmx_file* pmx, Engine::FSReader* reader) {
	pmx_texture texture;
	texture.path = _pmx_readText(reader);
	pmx->textures[id] = texture;
}

static void _pmx_readMaterial(Sint32 id, pmx_file* pmx, Engine::FSReader* reader) {
	pmx_material material;
	SDL_memset(&material, 0, sizeof(pmx_material));

	material.name = _pmx_readText(reader);
	material.name_en = _pmx_readText(reader);

	reader->Read4F32(material.diffuse_color);
	reader->Read3F32(material.specular_color);
	material.specular_strength = reader->ReadF32();
	reader->Read3F32(material.ambient_color);
	material.drawing_flags = reader->ReadU8();
	reader->Read4F32(material.edge_color);
	material.edge_scale = reader->ReadF32();

	material.texture_id = _pmx_readUINT(pmx->header.g_texture_index_size, reader);
	material.texture_id_env = _pmx_readSINT(pmx->header.g_texture_index_size, reader);
	material.env_blend_mode = reader->ReadU8();
	material.toon_reference = reader->ReadU8();
	if (material.toon_reference) {
		material.toon_value = reader->ReadU8();
	}
	else {
		material.toon_value = _pmx_readSINT(pmx->header.g_texture_index_size, reader);
	}

	material.comment = _pmx_readText(reader);
	material.surface_count = reader->ReadU32();

	pmx->materials[id] = material;
}

static void _pmx_readBone(Sint32 id, pmx_file* pmx, Engine::FSReader* reader) {
	pmx_bone bone;

	bone.name = _pmx_readText(reader);
	bone.name_en = _pmx_readText(reader);
	reader->Read3F32(bone.position);
	bone.parent_id = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
	bone.layer = reader->ReadU32();
	bone.flags = reader->ReadU16();

	if (bone.flags & PMX_BONEFLAG_INDEXED_TAIL) { // Offset position
		bone.connect_index = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
	}
	else {
		reader->Read3F32(bone.tail_pos);
	}

	// Inherit bone
	if ((bone.flags & PMX_BONEFLAG_INHERIT_ROTATION) || (bone.flags & PMX_BONEFLAG_INHERIT_TRANSLATION)) {
		bone.inh_index = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		bone.inh_influence = reader->ReadF32();
	}

	// Fixed axis
	if (bone.flags & PMX_BONEFLAG_FIXED_AXIS) {
		reader->Read3F32(bone.fixed_axis);
	}

	// Loacal co-ordinate
	if (bone.flags & PMX_BONEFLAG_LOCAL_COORDINATE) {
		reader->Read3F32(bone.local_x);
		reader->Read3F32(bone.local_z);
	}

	// External parent
	if (bone.flags & PMX_BONEFLAG_EXTERNAL_PARENT_DEFORM) {
		bone.key = reader->ReadU32();
	}

	// IK
	if (bone.flags & PMX_BONEFLAG_IK) {
		bone.ik_target_index = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
		bone.ik_loop_count = reader->ReadU32();
		bone.ik_limit_radian = reader->ReadF32();
		bone.ik_link_count = reader->ReadU32();
		bone.ik_links = (pmx_ik_link*)rg_malloc(sizeof(pmx_ik_link) * bone.ik_link_count);

		//IK Links
		for (Uint32 i = 0; i < bone.ik_link_count; ++i) {
			bone.ik_links[i].bone_index = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
			bone.ik_links[i].has_limits = reader->ReadU8();
			if (bone.ik_links[i].has_limits) { // IK Angle limits
				reader->Read3F32(bone.ik_links[i].min);
				reader->Read3F32(bone.ik_links[i].max);
			}
		}
	}

	pmx->bones[id] = bone;
}

static void _pmx_readMorphGroup(Sint32 id, pmx_file* pmx, pmx_morph* morph, Engine::FSReader* reader) {
	morph->offsets[id].morph_index = _pmx_readSINT(pmx->header.g_morph_index_size, reader);
	morph->offsets[id].morph_weight = reader->ReadF32();
}

static void _pmx_readMorphVertex(Sint32 id, pmx_file* pmx, pmx_morph* morph, Engine::FSReader* reader) {
	morph->offsets[id].vertex_index = _pmx_readSINT(pmx->header.g_vertex_index_size, reader);
	reader->Read3F32(morph->offsets[id].vertex_pos);
}

static void _pmx_readMorphBone(Sint32 id, pmx_file* pmx, pmx_morph* morph, Engine::FSReader* reader) {
	morph->offsets[id].bone_index = _pmx_readSINT(pmx->header.g_bone_index_size, reader);
	reader->Read3F32(morph->offsets[id].bone_pos);
	reader->Read4F32(morph->offsets[id].bone_rot);
}

static void _pmx_readMorphUV(Sint32 id, pmx_file* pmx, pmx_morph* morph, Engine::FSReader* reader) {
	morph->offsets[id].uv_index = _pmx_readSINT(pmx->header.g_vertex_index_size, reader);
	reader->Read4F32(morph->offsets[id].uv_floats);
}

static void _pmx_readMorphMaterial(Sint32 id, pmx_file* pmx, pmx_morph* morph, Engine::FSReader* reader) {
	morph->offsets[id].material_index = _pmx_readSINT(pmx->header.g_material_index_size, reader);
	reader->ReadU8(); // Unknown byte
	reader->Read4F32(morph->offsets[id].material_diffuse);
	reader->Read3F32(morph->offsets[id].material_specular);
	morph->offsets[id].material_shininess = reader->ReadF32();
	reader->Read3F32(morph->offsets[id].material_ambient);
	reader->Read4F32(morph->offsets[id].material_edgecolor);
	morph->offsets[id].material_edgesize = reader->ReadF32();
	reader->Read4F32(morph->offsets[id].material_texture_tint);
	reader->Read4F32(morph->offsets[id].material_env_tint);
	reader->Read4F32(morph->offsets[id].material_toon_tint);
}

static void _pmx_readMorphFlip(Sint32 id, pmx_file* pmx, pmx_morph* morph, Engine::FSReader* reader) {
	morph->offsets[id].flip_index = _pmx_readSINT(pmx->header.g_morph_index_size, reader);
	morph->offsets[id].flip_weight = reader->ReadF32();
}

static void _pmx_readMorphImpulse(Sint32 id, pmx_file* pmx, pmx_morph* morph, Engine::FSReader* reader) {
	morph->offsets[id].imp_index = _pmx_readSINT(pmx->header.g_rigidbody_index_size, reader);
	morph->offsets[id].imp_local_flag = reader->ReadU8();
	reader->Read3F32(morph->offsets[id].imp_speed);
	reader->Read3F32(morph->offsets[id].imp_torque);
}

static void _pmx_readMorph(Sint32 id, pmx_file* pmx, Engine::FSReader* reader) {
	pmx_morph morph;

	morph.name = _pmx_readText(reader);
	morph.name_en = _pmx_readText(reader);

	morph.panel_type = reader->ReadU8();
	morph.morph_type = reader->ReadU8();

	morph.offset_count = reader->ReadU32();
	morph.offsets = (pmx_morph_offset*)rg_malloc(sizeof(pmx_morph_offset) * morph.offset_count);
	for (Uint32 i = 0; i < morph.offset_count; ++i) {
		switch (morph.morph_type) {
		case PMX_MORPH_GROUP:
			_pmx_readMorphGroup(i, pmx, &morph, reader);
			break;
		case PMX_MORPH_VERTEX:
			_pmx_readMorphVertex(i, pmx, &morph, reader);
			break;
		case PMX_MORPH_BONE:
			_pmx_readMorphBone(i, pmx, &morph, reader);
			break;
		case PMX_MORPH_UV:
		case PMX_MORPH_UV1:
		case PMX_MORPH_UV2:
		case PMX_MORPH_UV3:
		case PMX_MORPH_UV4:
			_pmx_readMorphUV(i, pmx, &morph, reader);
			break;
		case PMX_MORPH_MATERIAL:
			_pmx_readMorphMaterial(i, pmx, &morph, reader);
			break;
		case PMX_MORPH_FLIP:
			_pmx_readMorphFlip(i, pmx, &morph, reader);
			break;
		case PMX_MORPH_IMPULSE:
			_pmx_readMorphImpulse(i, pmx, &morph, reader);
			break;
		default:
			break;
		}
	}

	pmx->morphs[id] = morph;
}

pmx_file* pmx_load(String path) {
	Engine::FSReader* reader = new Engine::FSReader(path);
	bool bVertexSDEF = false;
	char string_buffer[128];

	pmx_file* pmxFile = (pmx_file*)rg_malloc(sizeof(pmx_file));
	rgLogInfo(RG_LOG_SYSTEM, "PMX: Loading model: %s", path);

	Engine::FS_PathFrom(pmxFile->path, path, 256);

	// Read header
	_pmx_header header;
	reader->Read(&header, sizeof(_pmx_header));
	if (header.signature[0] != 'P' ||
		header.signature[1] != 'M' ||
		header.signature[2] != 'X' ||
		header.signature[3] != ' ') {
		SDL_snprintf(string_buffer, 128, "%s is not a Polygon Model eXtended file!", path);
		delete reader;
		return NULL;
	}

	pmxFile->header.signature[0] = header.signature[0];
	pmxFile->header.signature[1] = header.signature[1];
	pmxFile->header.signature[2] = header.signature[2];
	pmxFile->header.signature[3] = header.signature[3];
	pmxFile->header.version = header.version;
	pmxFile->header.globals_count = header.globals_count;
	pmxFile->header.g_text_encoding = header.g_text_encoding;
	pmxFile->header.g_additional_vec4_count = header.g_additional_vec4_count;
	pmxFile->header.g_vertex_index_size = header.g_vertex_index_size;
	pmxFile->header.g_texture_index_size = header.g_texture_index_size;
	pmxFile->header.g_material_index_size = header.g_material_index_size;
	pmxFile->header.g_bone_index_size = header.g_bone_index_size;
	pmxFile->header.g_morph_index_size = header.g_morph_index_size;
	pmxFile->header.g_rigidbody_index_size = header.g_rigidbody_index_size;
	pmxFile->header.model_name = _pmx_readText(reader);
	pmxFile->header.model_name_en = _pmx_readText(reader);
	pmxFile->header.model_comment = _pmx_readText(reader);
	pmxFile->header.model_comment_en = _pmx_readText(reader);

#if PMX_DEBUG
	char str_buffer[512];
	char line_buffer[128];
	SDL_strlcpy(str_buffer, "~~~ PMX HEADER ~~~\n", 512);
	SDL_snprintf(line_buffer, 128, "Text encoding:         %d", pmxFile->header.g_text_encoding);
	SDL_strlcat(str_buffer, line_buffer, 512);

	if (pmxFile->header.g_text_encoding) {
		SDL_snprintf(line_buffer, 128, " (UTF-8)\n");
	}
	else {
		SDL_snprintf(line_buffer, 128, " (UTF-16LE)\n");
	}
	SDL_strlcat(str_buffer, line_buffer, 512);

	SDL_snprintf(line_buffer, 128, "Additional vec4:       %d\n", pmxFile->header.g_additional_vec4_count);
	SDL_strlcat(str_buffer, line_buffer, 512);
	SDL_snprintf(line_buffer, 128, "Vertex index size:     %d\n", pmxFile->header.g_vertex_index_size);
	SDL_strlcat(str_buffer, line_buffer, 512);
	SDL_snprintf(line_buffer, 128, "Texture index size:    %d\n", pmxFile->header.g_texture_index_size);
	SDL_strlcat(str_buffer, line_buffer, 512);
	SDL_snprintf(line_buffer, 128, "Material index size:   %d\n", pmxFile->header.g_material_index_size);
	SDL_strlcat(str_buffer, line_buffer, 512);
	SDL_snprintf(line_buffer, 128, "Bone index size:       %d\n", pmxFile->header.g_bone_index_size);
	SDL_strlcat(str_buffer, line_buffer, 512);
	SDL_snprintf(line_buffer, 128, "Morph index size:      %d\n", pmxFile->header.g_morph_index_size);
	SDL_strlcat(str_buffer, line_buffer, 512);
	SDL_snprintf(line_buffer, 128, "Rigid body index size: %d\n", pmxFile->header.g_rigidbody_index_size);
	SDL_strlcat(str_buffer, line_buffer, 512);
	rgLogInfo(RG_LOG_SYSTEM, "PMX DEBUG: %s", str_buffer);

	//SDL_memset(line_buffer, 0, 128);
	//SDL_memcpy(line_buffer, pmxFile->header.model_name.data, pmxFile->header.model_name.len);
	
	UTF8_FromUTF16((WString)pmxFile->header.model_name.data, pmxFile->header.model_name.len);
	rgLogInfo(RG_LOG_SYSTEM, "PMX Name: %s", UTF8_GetBuffer());

#endif // PMX_DEBUG

	// Read vertices
	pmxFile->vertex_count = reader->ReadU32();

	rgLogInfo(RG_LOG_SYSTEM, "PMX: Loading vertices");
	rgLogInfo(RG_LOG_SYSTEM, "PMX: Vertices %d", pmxFile->vertex_count);

	pmxFile->vertices = (pmx_vertex*)rg_malloc(sizeof(pmx_vertex) * pmxFile->vertex_count);
	for (Uint32 i = 0; i < pmxFile->vertex_count; ++i) {
		_pmx_readVertex(i, pmxFile, reader, &bVertexSDEF);
	}

	// Read indices
	rgLogInfo(RG_LOG_SYSTEM, "PMX: Loading indices");
	pmxFile->index_count = reader->ReadU32();
	rgLogInfo(RG_LOG_SYSTEM, "PMX: indices %d", pmxFile->index_count);
	pmxFile->indices = rg_malloc(pmxFile->header.g_vertex_index_size * pmxFile->index_count);
	reader->Read(pmxFile->indices, pmxFile->header.g_vertex_index_size * pmxFile->index_count);

	// Read textures
	rgLogInfo(RG_LOG_SYSTEM, "PMX: Loading textures table");
	pmxFile->texture_count = reader->ReadU32();
	rgLogInfo(RG_LOG_SYSTEM, "PMX: textures %d", pmxFile->texture_count);
	pmxFile->textures = (pmx_texture*)rg_malloc(sizeof(pmx_texture) * pmxFile->texture_count);
	for (Uint32 i = 0; i < pmxFile->texture_count; ++i) {
		_pmx_readTexture(i, pmxFile, reader);
	}

	// Read materials
	rgLogInfo(RG_LOG_SYSTEM, "PMX: Loading materials");
	pmxFile->material_count = reader->ReadU32();
	rgLogInfo(RG_LOG_SYSTEM, "PMX: materials %d", pmxFile->material_count);
	pmxFile->materials = (pmx_material*)rg_malloc(sizeof(pmx_material) * pmxFile->material_count);
	for (Uint32 i = 0; i < pmxFile->material_count; ++i) {
		_pmx_readMaterial(i, pmxFile, reader);
	}

	// Read bones
	rgLogInfo(RG_LOG_SYSTEM, "PMX: Loading bones");
	pmxFile->bone_count = reader->ReadU32();
	rgLogInfo(RG_LOG_SYSTEM, "PMX: bones %d", pmxFile->bone_count);
	pmxFile->bones = (pmx_bone*)rg_malloc(sizeof(pmx_bone) * pmxFile->bone_count);
	for (Uint32 i = 0; i < pmxFile->bone_count; ++i) {
		_pmx_readBone(i, pmxFile, reader);
	}

	rgLogInfo(RG_LOG_SYSTEM, "PMX: Loading morphs");
	pmxFile->morph_count = reader->ReadU32();
	rgLogInfo(RG_LOG_SYSTEM, "PMX: morphs %d", pmxFile->morph_count);
	pmxFile->morphs = (pmx_morph*)rg_malloc(sizeof(pmx_morph) * pmxFile->morph_count);
	for (Uint32 i = 0; i < pmxFile->morph_count; ++i) {
		_pmx_readMorph(i, pmxFile, reader);
	}

	delete reader;

	return pmxFile;
}

void pmx_free(pmx_file* ptr) {
	rg_free(ptr->header.model_name.data);
	rg_free(ptr->header.model_name_en.data);
	rg_free(ptr->header.model_comment.data);
	rg_free(ptr->header.model_comment_en.data);
	for (Uint32 i = 0; i < ptr->vertex_count; ++i) {
		if (ptr->vertices[i].additional_vec4) {
			rg_free(ptr->vertices[i].additional_vec4);
		}
	}
	rg_free(ptr->vertices);

	rg_free(ptr->indices);

	for (Uint32 i = 0; i < ptr->texture_count; ++i) {
		rg_free(ptr->textures[i].path.data);
	}
	rg_free(ptr->textures);

	for (Uint32 i = 0; i < ptr->material_count; ++i) {
		rg_free(ptr->materials[i].name.data);
		rg_free(ptr->materials[i].name_en.data);
		rg_free(ptr->materials[i].comment.data);
	}
	rg_free(ptr->materials);

	for (Uint32 i = 0; i < ptr->bone_count; ++i) {
		rg_free(ptr->bones[i].name.data);
		rg_free(ptr->bones[i].name_en.data);
		//		if(ptr->bones[i].ik_link_count) {
		//			free(ptr->bones[i].ik_links);
		//		}
	}
	rg_free(ptr->bones);

	rg_free(ptr);
}