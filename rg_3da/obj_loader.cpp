#include "obj_loader.h"

#if CUSTOM_OBJ_ENABLED

#include <rgmath.h>
#include <filesystem.h>
#include <rgstring.h>

#include <engine.h>
#include <allocator.h>

#include <vector>

struct ObjParseState {
    Engine::FSReader* reader;
    R3D_Vertex*       vertices;
    Uint32*           indices;
    Uint32            p_index;
    Uint32            t_index;
    char              tmp_string[128];
    char              mtllib[128];
};

struct ObjMaterial {
    Uint32 name_hash;
    vec3   color;
    char   name[128];
    char   diffuse[128];
    char   normal[128];
    char   pbr[128];
};

struct ObjMesh {
    Uint32 mathash;
    Uint32 material;
    Uint32 indices;
};

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

static void ProcessFloats(char* str, Float32* vtx, Bool proc2F) {
    Sint32 s_pos = Engine::rg_strcharats(str, ' ');
    str[s_pos] = '\0';
    double d0 = SDL_atof(str);
    str = &str[s_pos + 1];
    s_pos = Engine::rg_strcharats(str, ' ');
    str[s_pos] = '\0';
    double d1 = SDL_atof(str);
    vtx[0] = (float)d0;
    vtx[1] = (float)d1;

    if (!proc2F) {
        str = &str[s_pos + 1];
        double d2 = SDL_atof(str);
        vtx[2] = (float)d2;
    }
}

static void ParseVertex(char* vStr, std::vector<Float32>& vertices, std::vector<Float32>& normals, std::vector<Float32>& uvs) {
    Float32 v[3];
    if (Engine::rg_strstw(vStr, "v ")) {
        ProcessFloats(&vStr[2], v, false);
        vertices.push_back(v[0]);
        vertices.push_back(v[1]);
        vertices.push_back(v[2]);
    }
    else if (Engine::rg_strstw(vStr, "vn ")) {
        ProcessFloats(&vStr[3], v, false);
        normals.push_back(v[0]);
        normals.push_back(v[1]);
        normals.push_back(v[2]);
    }
    else if (Engine::rg_strstw(vStr, "vt ")) {
        ProcessFloats(&vStr[3], v, true);
#if 1
        uvs.push_back(v[0]);
        uvs.push_back(1.0f - v[1]);
#else
        Sint32 iu = (Sint32)v[0];
        Sint32 iv = (Sint32)v[1];
        Float32 tu = v[0] - iu;
        Float32 tv = v[1] - iv;
        //uvs.push_back(v[0]);
        //uvs.push_back(-v[1]);
        uvs.push_back(tu);
        uvs.push_back(1.0f - tv);
#endif
    }
}

static void ProcessIndex(ObjParseState* state, char* str, std::vector<Float32>& vertices, std::vector<Float32>& normals, std::vector<Float32>& uvs) {
    Uint32 p = 0;
    Uint32 sptr = 0;
    Uint32 vidx[3] = {}; // 0 - vertex, 1 - texture, 2 - normal
    char* pidx[3] = {};

    for (Uint32 i = 0;; i++) {
        char c = str[i];
        if (c == '/' || c == '\0') {
            str[i] = '\0';
            vidx[p] = SDL_atoi(&str[sptr]) - 1;
            pidx[p] = &str[sptr];
            sptr = i + 1;
            if (p == 2) { break; }
            p++;
        }
    }

    Uint32 idx = vidx[0];
    state->vertices[idx].pos.x = vertices[vidx[0] * 3 + 0];
    state->vertices[idx].pos.y = vertices[vidx[0] * 3 + 1];
    state->vertices[idx].pos.z = vertices[vidx[0] * 3 + 2];
    state->vertices[idx].uv.x = uvs[vidx[1] * 2 + 0];
    state->vertices[idx].uv.y = uvs[vidx[1] * 2 + 1];
    state->vertices[idx].norm.x = normals[vidx[2] * 3 + 0];
    state->vertices[idx].norm.y = normals[vidx[2] * 3 + 1];
    state->vertices[idx].norm.z = normals[vidx[2] * 3 + 2];

    state->indices[state->t_index] = idx;

    state->t_index++;
    state->p_index++;

    //rgLogInfo(RG_LOG_SYSTEM, "[OBJ] nums: %d / %d / %d => %p / %p / %p",
    //    vidx[0], vidx[1], vidx[2],
    //    pidx[0], pidx[1], pidx[2]);
}

static void ParseFace(ObjParseState* state, char* fStr, std::vector<Float32>& vertices, std::vector<Float32>& normals, std::vector<Float32>& uvs) {
    Uint32 len = SDL_strlen(fStr) + 1;
    Uint32 idx = 0;
    for (Uint32 i = 0; i < len; i++) {
        char c = fStr[i];
        if (c == ' ' || c == '\0') {
            fStr[i] = '\0';
            ProcessIndex(state, &fStr[idx], vertices, normals, uvs);
            idx = i + 1;
        }
    }

    // Calculate tangent
    Uint32 v0idx = state->indices[state->t_index - 3];
    Uint32 v1idx = state->indices[state->t_index - 2];
    Uint32 v2idx = state->indices[state->t_index - 1];
    R3D_Vertex* v0 = &state->vertices[v0idx];
    R3D_Vertex* v1 = &state->vertices[v1idx];
    R3D_Vertex* v2 = &state->vertices[v2idx];
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

static void FreeState(ObjParseState* state) {
    if (state->vertices != NULL) {
        rg_free(state->vertices);
    }
    if (state->indices != NULL) {
        rg_free(state->indices);
    }
    delete state->reader;
}

static Bool ProcessMTL(ObjParseState* state, std::vector<ObjMaterial*>& materials) {
    rgLogInfo(RG_LOG_SYSTEM, "[OBJ] +-> Loading mtl %s", state->mtllib);

    Engine::FSReader* reader = new Engine::FSReader(state->mtllib);

    Bool newmtl = false;
    ObjMaterial* mat = NULL;

    for (;;) {
        Bool isEOF = ReadString(reader, state->tmp_string);

        if (Engine::rg_strstw(state->tmp_string, "newmtl ")) {
            if (newmtl) {
                materials.push_back(mat);
            }
            newmtl = true;
            mat = new ObjMaterial;

            mat->diffuse[0] = 0;
            mat->normal[0] = 0;
            mat->pbr[0] = 0;

            SDL_snprintf(mat->name, 128, "%s", &state->tmp_string[7]);
            mat->name_hash = rgCRC32(mat->name, SDL_strlen(mat->name));

            // Diffuse color
        }
        else if (Engine::rg_strstw(state->tmp_string, "Kd ")) {
            if (!newmtl) {
                rgLogError(RG_LOG_SYSTEM, "[OBJ] No 'newmtl' defined!");
                FreeState(state);
                delete reader;
                return true;
            }
            ProcessFloats(&state->tmp_string[3], mat->color.array, false);

            // Diffuse texture
        }
        else if (Engine::rg_strstw(state->tmp_string, "map_Kd ")) {
            if (!newmtl) {
                rgLogError(RG_LOG_SYSTEM, "[OBJ] No 'newmtl' defined!");
                FreeState(state);
                delete reader;
                return true;
            }
            SDL_snprintf(mat->diffuse, 128, "%s", &state->tmp_string[7]);
            // Normal texture
        }
        else if (Engine::rg_strstw(state->tmp_string, "map_Disp ")) {
            if (!newmtl) {
                rgLogError(RG_LOG_SYSTEM, "[OBJ] No 'newmtl' defined!");
                FreeState(state);
                delete reader;
                return true;
            }
            SDL_snprintf(mat->normal, 128, "%s", &state->tmp_string[9]);
            // PBR texture
        }
        else if (Engine::rg_strstw(state->tmp_string, "map_Kpbr ")) {
            if (!newmtl) {
                rgLogError(RG_LOG_SYSTEM, "[OBJ] No 'newmtl' defined!");
                FreeState(state);
                delete reader;
                return true;
            }
            SDL_snprintf(mat->pbr, 128, "%s", &state->tmp_string[9]);
        }

        if (isEOF) { break; }
    }

    if (newmtl) {
        materials.push_back(mat);
    }

    delete reader;
    rgLogInfo(RG_LOG_SYSTEM, "[OBJ] +-> Loaded %ld materials", materials.size());
    return false;
}

static Uint32 FindMaterial(Uint32 hash, std::vector<ObjMaterial*>& materials) {
    Uint32 idx = 0;
    for (; idx < materials.size(); idx++) {
        if (materials[idx]->name_hash == hash) {
            return idx;
        }
    }
    return 0;
}

R3D_StaticModel* OBJ_ToModel(String p) {
    rgLogInfo(RG_LOG_SYSTEM, "[OBJ] Loading %s", p);

    Uint64 start = SDL_GetPerformanceCounter();

    char path[128];
    Engine::FS_PathFrom(path, p, 128);

    ObjParseState state;
    SDL_memset(&state, 0, sizeof(ObjParseState));

    state.reader = new Engine::FSReader(p);
    std::vector<ObjMaterial*> materials;
    std::vector<ObjMesh*> meshes;
    std::vector<Float32> vertices;
    std::vector<Float32> normals;
    std::vector<Float32> uvs;

    Uint32 index_count = 0;

    // Read ALL vertices
    for (Uint32 i = 0;; i++) {
        Bool isEOF = ReadString(state.reader, state.tmp_string);
        // Parse vertex
        if (Engine::rg_strstw(state.tmp_string, "mtllib ")) {
            if (state.mtllib[0] == 0) {
                SDL_snprintf(state.mtllib, 128, "%s/%s", path, &state.tmp_string[7]);
                if (ProcessMTL(&state, materials)) {
                    return NULL;
                }
            }
            else {
                rgLogError(RG_LOG_SYSTEM, "[OBJ] Error at %d: Doubled 'mtllib' key.", i);
                FreeState(&state);
                return NULL;
            }
        }
        else if (Engine::rg_strstw(state.tmp_string, "f ")) {
            index_count += 3;
        }
        else {
            ParseVertex(state.tmp_string, vertices, normals, uvs);
        }
        if (isEOF) { break; }
    }

    if (normals.size() == 0) { rgLogInfo(RG_LOG_SYSTEM, "No normals!"); }
    if (uvs.size() == 0) { rgLogInfo(RG_LOG_SYSTEM, "No UVs!"); }
    rgLogInfo(RG_LOG_SYSTEM, "[OBJ] v: %ld n: %ld t: %ld", vertices.size(), normals.size(), uvs.size());

    state.vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * vertices.size());
    state.indices = (Uint32*)rg_malloc(sizeof(Uint32) * index_count);

    // Read faces
    state.reader->Seek(0, RG_FS_SEEK_SET);
    ObjMesh* current_mesh = NULL;
    for (;;) {
        Bool isEOF = ReadString(state.reader, state.tmp_string);
        if (Engine::rg_strstw(state.tmp_string, "usemtl ")) {
            // Use material
            String mname = &state.tmp_string[7];
            Uint32 mhash = rgCRC32(mname, SDL_strlen(mname));

            ObjMesh* mesh = new ObjMesh;
            mesh->mathash = mhash;
            mesh->material = FindMaterial(mhash, materials);

            if (state.p_index != 0) {
                current_mesh->indices = state.p_index;
                meshes.push_back(current_mesh);
                state.p_index = 0;
            }

            current_mesh = mesh;

        }
        else if (Engine::rg_strstw(state.tmp_string, "f ")) {
            // Face
            ParseFace(&state, &state.tmp_string[2], vertices, normals, uvs);
        }
        if (isEOF) { break; }
    }

    if (state.p_index == 0) {
        rgLogError(RG_LOG_SYSTEM, "[OBJ] Invalid obj file!");
        FreeState(&state);
        return NULL;
    }

    current_mesh->indices = state.p_index;
    meshes.push_back(current_mesh);

    R3D_MeshInfo* meshInfo = (R3D_MeshInfo*)rg_malloc(sizeof(R3D_MeshInfo) * meshes.size());

    //char* textures[64] = {};
    //for (Uint32 i = 0; i < meshes.size(); i++) {
    //    textures[i] = (char*)rg_malloc(256);
    //}
    //modelInfo.textures = (String*)textures;

    char dtex_path[256];
    char ntex_path[256];
    char ptex_path[256];
    R3DCreateMaterialInfo matInfo = {};

    for (Uint32 i = 0; i < meshes.size(); i++) {
        ObjMaterial* mat = materials[meshes[i]->material];

        if (mat->diffuse[0] == 0) {
            SDL_snprintf(dtex_path, 256, "platform/textures/def_diffuse.png");
        } else {
            SDL_snprintf(dtex_path, 256, "%s/%s", path, mat->diffuse);
        }

        if (mat->normal[0] == 0) {
            SDL_snprintf(ntex_path, 256, "platform/textures/def_normal.png");
        } else {
            SDL_snprintf(ntex_path, 256, "%s/%s", path, mat->normal);
        }

        if (mat->pbr[0] == 0) {
            SDL_snprintf(ptex_path, 256, "platform/textures/def_pbr2.png");
        } else {
            SDL_snprintf(ptex_path, 256, "%s/%s", path, mat->pbr);
        }

        matInfo.color.r = 1;
        matInfo.color.g = 1;
        matInfo.color.b = 1;
        matInfo.albedo = dtex_path;
        matInfo.normal = ntex_path;
        matInfo.pbr    = ptex_path;

        meshInfo[i].material   = Engine::Render::R3D_CreateMaterial(&matInfo);
        meshInfo[i].indexCount = meshes[i]->indices;

        //SDL_memcpy((void*)modelInfo.textures[i], dtex_path, 256);
    }


    R3DCreateStaticModelInfo info = {};
    info.vCount   = vertices.size();
    info.vertices = state.vertices;
    info.iCount   = index_count;
    info.indices  = state.indices;
    info.iType    = RG_INDEX_U32;
    // TODO
    info.mCount = meshes.size();
    info.info   = meshInfo;

    R3D_StaticModel* ret = Engine::Render::R3D_CreateStaticModel(&info);

    for (Uint32 i = 0; i < materials.size(); i++) {
        delete materials[i];
    }
    for (Uint32 i = 0; i < meshes.size(); i++) {
        delete meshes[i];
    }
    //rg_free(modelInfo.idxCount);
    rg_free(meshInfo);
    FreeState(&state);

    //for (Uint32 i = 0; i < modelInfo.materials; i++) {
    //    rg_free(textures[i]);
   // }
    Uint64 t = (SDL_GetPerformanceCounter() - start) / (SDL_GetPerformanceFrequency() / 1000);
    rgLogInfo(RG_LOG_SYSTEM, "[OBJ] +-> Done! %dms", (int)t);

    return ret;
}

#endif