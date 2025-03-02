#define DLL_EXPORT
#include "pm2importer.h"
#include "pm2.h"

#include "filesystem.h"
#include "engine.h"
#include "allocator.h"
#include "rgstring.h"

#define RG_PM2_RECALCULATE_NORMALS  0
#define RG_PM2_RECALCULATE_TANGENTS 0

namespace Engine {

// Deprecated
#if 0
    static void ReadMaterialsV2(PM2_Header* header, R3D_MaterialInfo* materials, FSReader* reader, String model_root) {

        char str_buffer[128];
        for (Uint32 i = 0; i < header->materials; i++) {

            Uint32 len = reader->ReadU32();
            SDL_memset(str_buffer, 0, 128);
            reader->Read(str_buffer, len);
            SDL_snprintf(materials[i].albedo, 128, "%s/%s", model_root, str_buffer);

            len = reader->ReadU32();
            SDL_memset(str_buffer, 0, 128);
            reader->Read(str_buffer, len);
            SDL_snprintf(materials[i].normal, 128, "%s/%s", model_root, str_buffer);

            // PM2 V2 PBR Texture
#if 0
            Sint32 slash = Engine::rg_strcharate(str_buffer, '/');
            str_buffer[slash] = 0;
            SDL_snprintf(materials[i].pbr, 128, "%s/%s/pbr.png", model_root, str_buffer);
#else
            SDL_snprintf(materials[i].pbr, 128, "platform/textures/def_pbr.png");
#endif
            vec4 diffuse;
            reader->Read4F32(diffuse);
            materials[i].color = { diffuse.r, diffuse.g, diffuse.b };

            //mat_info.shininess = 8;
            //mat_info.flags = R3D_MATERIAL_CULL_BACK_FACE;

        }

    }

    static void ReadMaterialsV3(PM2_Header* header, R3D_MaterialInfo* materials, FSReader* reader, String model_root) {

        char str_buffer[128];
        for (Uint32 i = 0; i < header->materials; i++) {

            Uint32 len = reader->ReadU32();
            SDL_memset(str_buffer, 0, 128);
            reader->Read(str_buffer, len);
            SDL_snprintf(materials[i].albedo, 128, "%s/%s", model_root, str_buffer);

            len = reader->ReadU32();
            SDL_memset(str_buffer, 0, 128);
            reader->Read(str_buffer, len);
            SDL_snprintf(materials[i].normal, 128, "%s/%s", model_root, str_buffer);

            len = reader->ReadU32();
            SDL_memset(str_buffer, 0, 128);
            reader->Read(str_buffer, len);
            SDL_snprintf(materials[i].pbr, 128, "%s/%s", model_root, str_buffer);

            vec4 diffuse;
            reader->Read4F32(diffuse);
            materials[i].color = { diffuse.r, diffuse.g, diffuse.b };

            //mat_info.shininess = 8;
            //mat_info.flags = R3D_MATERIAL_CULL_BACK_FACE;

        }

    }
#endif

    static void ReadMaterialsV4(PM2_Header* header, R3D_MaterialInfo* materials, FSReader* reader, String model_root) {

        vec3 diffuse = { 1.0f, 1.0f, 1.0f, };

        char albedo[128];
        char normal[128];
        char pbr[128];

        char str_buffer[128];
        for (Uint32 i = 0; i < header->materials; i++) {

            Uint32 len = reader->ReadU32();
            SDL_memset(str_buffer, 0, 128);
            reader->Read(str_buffer, len);

            // %gamedata%/textures/%texture%
            SDL_snprintf(materials[i].texture, 128, "%s", str_buffer);
#if 0
            SDL_snprintf(materials[i].albedo, 128, "%s/textures/%s.png",      GetGamedataPath(), str_buffer);
            SDL_snprintf(materials[i].normal, 128, "%s/textures/%s_bump.png", GetGamedataPath(), str_buffer);
            SDL_snprintf(materials[i].pbr,    128, "%s/textures/%s_pbr.png",  GetGamedataPath(), str_buffer);
#endif
            reader->Read3F32(diffuse);
            materials[i].color = { diffuse.r, diffuse.g, diffuse.b };

        }

    }

    ////////////////////////////////////////
    // Static sort functions

    static void SortMeshInfo(R3D_MatMeshInfo* meshinfo, Uint32 size) {
        Bool swapped;
        Sint32 i, j;

        R3D_MatMeshInfo buffer;

        for (i = 0; i < size - 1; i++) {
            swapped = false;
            for (j = 0; j < size - i - 1; j++) {
                if (meshinfo[j].materialIdx > meshinfo[j + 1].materialIdx) {
                    // Swap
                    buffer = meshinfo[j];
                    meshinfo[j] = meshinfo[j + 1];
                    meshinfo[j + 1] = buffer;

                    swapped = true;
                }
            }

            if (!swapped) { break; }
        }
    }


    ////////////////////////////////////////

    void PM2Importer::ImportModel(String p, R3DStaticModelInfo* info) {

        char path[256];
        SDL_memset(path, 0, 256);
        FS_ReplaceSeparators(path, p);

        FSReader* reader = new FSReader(path);

        char model_root[256];
        FS_PathFrom(model_root, path, 256);

        PM2_Header header;
        reader->Read(&header, sizeof(PM2_Header));

        if (header.sig[0] != 'P' ||
            header.sig[1] != 'M' ||
            header.sig[2] != '2' ||
            header.sig[3] != ' ') {
            char buffer[128];
            SDL_snprintf(buffer, 128, "%s is not a PM2 file!", path);
            RG_ERROR_MSG(buffer);
        }

        //if (header.version < 2 && header.version > 3) {
        if (header.version < 4) {
            RG_ERROR_MSG("Unsupported pm2 version!");
        }

        IndexType index_type = RG_INDEX_U16;
        if (RG_CHECK_FLAG(header.flags, PM2_FLAG_EXTENDED_INDICES)) {
            index_type = RG_INDEX_U32;
        }

        PM2_MeshInfo* mesh_info = (PM2_MeshInfo*)rg_malloc(sizeof(PM2_MeshInfo) * header.offset); // offset - mesh count in pm2 v2
        PM2_Vertex* vertices = (PM2_Vertex*)rg_malloc(sizeof(PM2_Vertex) * header.vertices);

        R3D_MatMeshInfo* r3d_meshinfo = (R3D_MatMeshInfo*)rg_malloc(sizeof(R3D_MatMeshInfo) * header.offset);
        R3D_Vertex* r3d_vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * header.vertices);
        void* indices = rg_malloc(index_type * header.indices);

        //Uint32 materials[header.materials];
        R3D_MaterialInfo* materials = (R3D_MaterialInfo*)rg_malloc(sizeof(R3D_MaterialInfo) * header.materials);

        switch (header.version) {
#if 0
            case 2: { ReadMaterialsV2(&header, materials, reader, model_root); break; }
            case 3: { ReadMaterialsV3(&header, materials, reader, model_root); break; }
#endif
            case 4: { ReadMaterialsV4(&header, materials, reader, model_root); break; }
            default: { RG_ERROR_MSG("PM2: Unknown file format!") break; }
        }

        reader->Read(mesh_info, sizeof(PM2_MeshInfo) * header.offset);
        reader->Read(vertices, sizeof(PM2_Vertex) * header.vertices);
        reader->Read(indices, index_type * header.indices);

        // To R3D_Model
        Uint32 idx_offset = 0;
        for (Uint32 i = 0; i < header.offset; i++) {
            r3d_meshinfo[i].indexCount  = mesh_info[i].indices;
            r3d_meshinfo[i].indexOffset = idx_offset;
            r3d_meshinfo[i].materialIdx = mesh_info[i].material;

            idx_offset += r3d_meshinfo[i].indexCount;

        }

        // Sort meshes
        SortMeshInfo(r3d_meshinfo, header.offset);


        AABB aabb = { {10000, 10000, 10000}, {-10000, -10000, -10000} };

        for (Uint32 i = 0; i < header.vertices; i++) {

            vec3* c_pos = &vertices[i].position;

            if (c_pos->x < aabb.min.x) { aabb.min.x = c_pos->x; }
            if (c_pos->y < aabb.min.y) { aabb.min.y = c_pos->y; }
            if (c_pos->z < aabb.min.z) { aabb.min.z = c_pos->z; }
            if (c_pos->x > aabb.max.x) { aabb.max.x = c_pos->x; }
            if (c_pos->y > aabb.max.y) { aabb.max.y = c_pos->y; }
            if (c_pos->z > aabb.max.z) { aabb.max.z = c_pos->z; }

            r3d_vertices[i].pos.x  = c_pos->x;
            r3d_vertices[i].pos.y  = c_pos->y;
            r3d_vertices[i].pos.z  = c_pos->z;
            r3d_vertices[i].norm.x = vertices[i].normal.x;
            r3d_vertices[i].norm.y = vertices[i].normal.y;
            r3d_vertices[i].norm.z = vertices[i].normal.z;
            r3d_vertices[i].tang.x = vertices[i].tangent.x;
            r3d_vertices[i].tang.y = vertices[i].tangent.y;
            r3d_vertices[i].tang.z = vertices[i].tangent.z;
            r3d_vertices[i].uv.x   = vertices[i].uv.x;
            r3d_vertices[i].uv.y   = vertices[i].uv.y;
        }

#if RG_PM2_RECALCULATE_NORMALS
        // Recalculate tangents
        for (Uint32 i = 0; i < header.indices / 3; i += 3) {

            Uint32 v0idx = 0;
            Uint32 v1idx = 0;
            Uint32 v2idx = 0;

            if (RG_CHECK_FLAG(header.flags, PM2_FLAG_EXTENDED_INDICES)) {
                Uint32* i32 = (Uint32*)indices;
                v0idx = i32[i + 0];
                v1idx = i32[i + 1];
                v2idx = i32[i + 2];
            } else {
                Uint16* i16 = (Uint16*)indices;
                v0idx = i16[i + 0];
                v1idx = i16[i + 1];
                v2idx = i16[i + 2];
            }

            R3D_Vertex* v0 = &r3d_vertices[v0idx];
            R3D_Vertex* v1 = &r3d_vertices[v1idx];
            R3D_Vertex* v2 = &r3d_vertices[v2idx];

            vec3 p0 = v1->pos - v0->pos;
            vec3 p1 = v2->pos - v0->pos;
#if 0
            vec3 np0 = p0.normalize_safe();
            vec3 np1 = p1.normalize_safe();
            vec3 nN = np0.cross(np1);
#endif
            vec3 nN = p0.cross(p1);

            vec3 N = nN.normalize_safe();

            v0->norm = N;
            v1->norm = N;
            v2->norm = N;

        }
#endif

#if RG_PM2_RECALCULATE_TANGENTS
        // Recalculate tangents
        for (Uint32 i = 0; i < header.indices / 3; i += 3) {

            Uint32 v0idx = 0;
            Uint32 v1idx = 0;
            Uint32 v2idx = 0;

            if (RG_CHECK_FLAG(header.flags, PM2_FLAG_EXTENDED_INDICES)) {
                Uint32* i32 = (Uint32*)indices;
                v0idx = i32[i + 0];
                v1idx = i32[i + 1];
                v2idx = i32[i + 2];
            } else {
                Uint16* i16 = (Uint16*)indices;
                v0idx = i16[i + 0];
                v1idx = i16[i + 1];
                v2idx = i16[i + 2];
            }

            R3D_Vertex* v0 = &r3d_vertices[v0idx];
            R3D_Vertex* v1 = &r3d_vertices[v1idx];
            R3D_Vertex* v2 = &r3d_vertices[v2idx];
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
#endif

#if 0
        typedef struct R3DCreateStaticModelInfo {
            R3D_MeshInfo* info;
            R3D_Vertex* vertices;
            void* indices;
            Uint32        vCount;
            Uint32        iCount;
            Uint32        mCount;
            IndexType     iType;
        } R3DCreateStaticModelInfo;
#endif


        // Materials
        info->matInfo  = materials;
        info->matCount = header.materials;

        // Meshes
        info->mInfo  = r3d_meshinfo;
        info->mCount = header.offset;

        // Data
        info->vertices = r3d_vertices;
        info->vCount   = header.vertices;
        info->indices  = indices;
        info->iCount   = header.indices;
        info->iType    = index_type;

        info->aabb     = aabb;


        delete reader;
        rg_free(mesh_info);
        rg_free(vertices);

	}

	void PM2Importer::FreeModelData(R3DStaticModelInfo* info) {
        rg_free(info->mInfo);
        rg_free(info->matInfo);
        rg_free(info->vertices);
        rg_free(info->indices);
	}

}