#include "ksmimporter.h"

#include <rgmath.h>
#include <rgstring.h>
#include <filesystem.h>
#include <allocator.h>
#include <map>

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
//#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#define PACK( __Declaration__ ) __Declaration__
#endif

#pragma pack(push, 1)
PACK(struct vertex_3d {
    /** @brief The position of the vertex */
    vec3 position;
    /** @brief The normal of the vertex. */
    vec3 normal;
    /** @brief The texture coordinate of the vertex. */
    vec2 texcoord;
    /** @brief The colour of the vertex. */
    vec4 colour;
    /** @brief The tangent of the vertex. */
    vec3 tangent;
});
#pragma pack(pop)

typedef struct mesh {
    Uint32 v_count;
    Uint32 i_count;
    vertex_3d* vertices;
    Uint32* indices;
    Uint32 mat_hash;
    R3D_Material* mat_idx;
} mesh;

static Bool ReadString(Engine::FSReader* reader, char* tmp_string) {
    char c;
    for (int i = 0;;) {
        c = reader->ReadU8();
        if (reader->EndOfStream()) {
            tmp_string[i] = '\0';
            return true;
        }
        if (c == '\r') { continue; }
        if (c == '\n') {
            tmp_string[i] = '\0';
            break;
        }
        else {
            tmp_string[i] = c;
            i++;
        }
    }
    return false;
}

static std::map<Uint32, R3D_Material*> loaded_materials; // mat_hash -> r3d_mat

#define TEXTURE_EXT "png"

static R3D_Material* GetMaterial(String mname, Uint32 mhash, String materials, String textures) {

    if (loaded_materials[mhash] != 0) { return loaded_materials[mhash]; }

    char path[256];
    SDL_snprintf(path, 256, "%s/%s.kmt", materials, mname);

    char albedo[256];
    char normal[256];
    char specular[256];

    albedo[0] = 0;
    normal[0] = 0;
    specular[0] = 0;

    Engine::FSReader* reader = new Engine::FSReader(path);
    char str[256];
    while (!reader->EndOfStream()) {
        if (ReadString(reader, str)) { break; }
        if (Engine::rg_strstw(str, "diffuse_map_name=")) {
            SDL_snprintf(albedo, 256, "%s/%s.%s", textures, &str[17], TEXTURE_EXT);
        }
        else if (Engine::rg_strstw(str, "normal_map_name=")) {
            SDL_snprintf(normal, 256, "%s/%s.%s", textures, &str[16], TEXTURE_EXT);
        }
        else if (Engine::rg_strstw(str, "specular_map_name=")) {
            SDL_snprintf(specular, 256, "%s/%s.%s", textures, &str[18], TEXTURE_EXT);
        }
    }


    if (albedo[0] == 0) {
        SDL_snprintf(albedo, 256, "platform/textures/def_diffuse.png");
    }
    if (normal[0] == 0) {
        SDL_snprintf(normal, 256, "platform/textures/def_normal.png");
    }
    if (specular[0] == 0) {
        SDL_snprintf(specular, 256, "platform/textures/def_pbr.png");
    }

    delete reader;

    rgLogInfo(RG_LOG_SYSTEM, "[KSM] Textures: %s %s %s", albedo, normal, specular);

    R3DCreateMaterialInfo mat_info = {};
    mat_info.albedo = albedo;
    mat_info.normal = normal;
    mat_info.pbr    = specular;
    mat_info.color = { 1, 1, 1 };

    loaded_materials[mhash] = Engine::Render::R3D_CreateMaterial(&mat_info);

    return loaded_materials[mhash];

}

void KSMImporter::ImportModel(String model, R3DCreateStaticModelInfo* info) {
    rgLogInfo(RG_LOG_SYSTEM, "[KSM] Assets path: %s", assets);

    char MODELS[256];
    char MATERIALS[256];
    char TEXTURES[256];
    char path[256];
    SDL_snprintf(MODELS, 256, "%s/models", assets);
    SDL_snprintf(MATERIALS, 256, "%s/materials", assets);
    SDL_snprintf(TEXTURES, 256, "%s/textures", assets);
    SDL_snprintf(path, 256, "%s/%s.ksm", MODELS, model);

    rgLogInfo(RG_LOG_SYSTEM, "[KSM] Loading: %s", path);
    Engine::FSReader* reader = new Engine::FSReader(path);

    Uint16 version = reader->ReadU16();
    Uint32 namelen = reader->ReadU32();
    char name[256];
    reader->Read(name, namelen);
    Uint32 meshcount = reader->ReadU32();

    mesh* meshes = (mesh*)rg_malloc(meshcount * sizeof(mesh));

    rgLogInfo(RG_LOG_SYSTEM, "[KSM] Name: %s, ver: %d, mesh count: %d", name, version, meshcount);

    Uint32 total_vertices = 0;
    Uint32 total_indices = 0;

    for (Uint32 i = 0; i < meshcount; i++) {
        mesh m = {};

        // Vertices
        Uint32 v_size = reader->ReadU32();
        m.v_count = reader->ReadU32();
        m.vertices = (vertex_3d*)rg_malloc(v_size * m.v_count);


        rgLogInfo(RG_LOG_SYSTEM, "[KSM] Size: %d / %d => %d", sizeof(vertex_3d), v_size, m.v_count);


        reader->Read(m.vertices, v_size * m.v_count);
        total_vertices += m.v_count;

        //rgLogInfo(RG_LOG_SYSTEM, "SIZE: %d/%d", v_size, sizeof(vertex_3d));

        // Indices
        Uint32 i_size = reader->ReadU32();
        m.i_count = reader->ReadU32();
        m.indices = (Uint32*)rg_malloc(i_size * m.i_count);
        reader->Read(m.indices, i_size * m.i_count);
        total_indices += m.i_count;

        // Name
        Uint32 strl = reader->ReadU32();
        char strn[256];
        reader->Read(strn, strl);

        // Material name
        Uint32 m_strl = reader->ReadU32();
        char mstrn[256];
        reader->Read(mstrn, m_strl);
        m.mat_hash = rgCRC32(mstrn, m_strl);

        // NOT USED
        // Center
        vec3 v;
        reader->Read3F32(v);
        // Min/max
        reader->Read3F32(v);
        reader->Read3F32(v);

        // Load material
        m.mat_idx = GetMaterial(mstrn, m.mat_hash, MATERIALS, TEXTURES);


        rgLogInfo(RG_LOG_SYSTEM, "[KSM] Loaded: %x %s, v %d, i %d", m.mat_hash, mstrn, m.v_count, m.i_count);
        meshes[i] = m;
    }

    delete reader;

    R3D_Vertex* vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * total_vertices);
    Uint32* indices = (Uint32*)rg_malloc(sizeof(Uint32) * total_indices);
    R3D_MeshInfo* meshinfo = (R3D_MeshInfo*)rg_malloc(sizeof(R3D_MeshInfo) * meshcount);

    Uint32 vtx = 0;
    Uint32 idx = 0;

    for (Uint32 i = 0; i < meshcount; i++) {
        mesh* m = &meshes[i];

        for (Uint32 j = 0; j < m->i_count; j++) {
            indices[idx] = m->indices[j] + vtx;
            idx++;
        }

        float* vtx_array = (float*)m->vertices;
        for (Uint32 j = 0; j < m->v_count; j++) {
#if 0
            vertices[vtx].pos  = m->vertices[j].position;
            vertices[vtx].norm = m->vertices[j].normal;
            vertices[vtx].tang = m->vertices[j].tangent;
            vertices[vtx].uv   = m->vertices[j].texcoord;
#else
            vertices[vtx].pos.x  = vtx_array[j*15 + 0];
            vertices[vtx].pos.y  = vtx_array[j*15 + 1];
            vertices[vtx].pos.z  = vtx_array[j*15 + 2];
            vertices[vtx].norm.x = vtx_array[j*15 + 3];
            vertices[vtx].norm.y = vtx_array[j*15 + 4];
            vertices[vtx].norm.z = vtx_array[j*15 + 5];
            vertices[vtx].uv.x   = vtx_array[j*15 + 6];
            vertices[vtx].uv.y   = vtx_array[j*15 + 7];
            /*
            float r = vtx_array[j * 15 + 8];
            float g = vtx_array[j * 15 + 9];
            float b = vtx_array[j * 15 + 10];
            float a = vtx_array[j * 15 + 11];
            */
            vertices[vtx].tang.x = vtx_array[j*15 + 12];
            vertices[vtx].tang.y = vtx_array[j*15 + 13];
            vertices[vtx].tang.z = vtx_array[j*15 + 14];
#endif
            vertices[vtx].uv.y = 1 - vertices[vtx].uv.y;
            vtx++;
        }

        meshinfo[i].indexCount = m->i_count;
        meshinfo[i].material   = loaded_materials[m->mat_hash];

    }

    info->vCount   = total_vertices;
    info->vertices = vertices;
    info->iCount   = total_indices;
    info->indices  = indices;
    info->iType    = RG_INDEX_U32;
    info->mCount   = meshcount;
    info->info     = meshinfo;

    // Free memory
    for (Uint32 i = 0; i < meshcount; i++) {
        rg_free(meshes[i].vertices);
        rg_free(meshes[i].indices);
    }
    rg_free(meshes);
}

void KSMImporter::FreeModelData(R3DCreateStaticModelInfo* info) {
    rg_free(info->vertices);
    rg_free(info->indices);
    rg_free(info->info);
}