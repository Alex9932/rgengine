#define DLL_EXPORT
#include "pm2importer.h"
#include "pm2.h"

#include "filesystem.h"
#include "engine.h"
#include "allocator.h"
#include "rgstring.h"

#include "render.h"

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

    static void ReadPM2String(char* buffer, Uint32 maxlen, FSReader* reader) {
        Uint32 len = reader->ReadU32();
        SDL_memset(buffer, 0, maxlen);
        reader->Read(buffer, SDL_min(len, maxlen - 1));
    }

    static void ReadMaterialsV4(PM2_Header* header, R3D_MaterialInfo* materials, FSReader* reader, String model_root) {

        vec3 diffuse = { 1.0f, 1.0f, 1.0f };

        char albedo[128];
        char normal[128];
        char pbr[128];

        char str_buffer[128];
        for (Uint32 i = 0; i < header->materials; i++) {

            ReadPM2String(str_buffer, 128, reader);

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

    static void ReadHeader(FSReader* reader, PM2_Header* header) {
        reader->Read(header, sizeof(PM2_Header));
        if (header->sig[0] != 'P' ||
            header->sig[1] != 'M' ||
            header->sig[2] != '2' ||
            header->sig[3] != ' ') {
            char buffer[128];
            SDL_snprintf(buffer, 128, "%s is not a PM2 file!", reader->GetResourcePath());
            RG_ERROR_MSG(buffer);
        }

        if (header->version < 4) {
            RG_ERROR_MSG("Unsupported pm2 version!");
        }
    }

    static void ReadSkeletonHeader(FSReader* reader, PM2_SkeletonHeader* skel_header) {
        reader->Read(skel_header, sizeof(PM2_SkeletonHeader));
        if (skel_header->sig[0] != 'P' ||
            skel_header->sig[1] != 'M' ||
            skel_header->sig[2] != '2' ||
            skel_header->sig[3] != 'S') {
            char buffer[128];
            SDL_snprintf(buffer, 128, "%s does not contain skeleton data!", reader->GetResourcePath());
            RG_ERROR_MSG(buffer);
        }
    }

    static void CopyMeshData(R3D_MatMeshInfo* dst, PM2_MeshInfo* src, Uint32 size) {
        // To R3D_Model
        Uint32 idx_offset = 0;
        for (Uint32 i = 0; i < size; i++) {
            dst[i].indexCount  = src[i].indices;
            dst[i].indexOffset = idx_offset;
            dst[i].materialIdx = src[i].material;
            idx_offset += dst[i].indexCount;
        }
	}

    static void CopyGeometryData(R3D_Vertex* dst, PM2_Vertex* src, AABB* aabb, Uint32 vertex_count) {

        for (Uint32 i = 0; i < vertex_count; i++) {
#if 0
            vec3* c_pos = &src[i].position;

            if (c_pos->x < aabb->min.x) { aabb->min.x = c_pos->x; }
            if (c_pos->y < aabb->min.y) { aabb->min.y = c_pos->y; }
            if (c_pos->z < aabb->min.z) { aabb->min.z = c_pos->z; }
            if (c_pos->x > aabb->max.x) { aabb->max.x = c_pos->x; }
            if (c_pos->y > aabb->max.y) { aabb->max.y = c_pos->y; }
            if (c_pos->z > aabb->max.z) { aabb->max.z = c_pos->z; }

            dst[i].pos.x  = c_pos->x;
            dst[i].pos.y  = c_pos->y;
            dst[i].pos.z  = c_pos->z;
#else
            dst[i].pos.x  = src[i].position.x;
            dst[i].pos.y  = src[i].position.y;
            dst[i].pos.z  = src[i].position.z;
#endif
            dst[i].norm.x = src[i].normal.x;
            dst[i].norm.y = src[i].normal.y;
            dst[i].norm.z = src[i].normal.z;
            dst[i].tang.x = src[i].tangent.x;
            dst[i].tang.y = src[i].tangent.y;
            dst[i].tang.z = src[i].tangent.z;
            dst[i].uv.x   = src[i].uv.x;
            dst[i].uv.y   = src[i].uv.y;
        }
    }

    static void CopyWeightData(R3D_Weight* dst, PM2_Weight* src, Uint32 vertex_count) {
        for (Uint32 i = 0; i < vertex_count; i++) {
            dst[i].weight.x = src[i].weights.x;
            dst[i].weight.y = src[i].weights.y;
            dst[i].weight.z = src[i].weights.z;
            dst[i].weight.w = src[i].weights.w;
            dst[i].idx.x = src[i].boneids.x;
            dst[i].idx.y = src[i].boneids.y;
            dst[i].idx.z = src[i].boneids.z;
            dst[i].idx.w = src[i].boneids.w;
        }
    }

    static void ReadBones(BoneInfo* bones, Uint32 count, FSReader* reader) {
        for (Uint32 i = 0; i < count; i++) {
            ReadPM2String(bones[i].name, 32, reader);
            bones[i].parent = reader->ReadS16();
            Uint16 flags = reader->ReadU16();
            if (RG_CHECK_FLAG(flags, PM2_BONE_FLAG_HAS_LIMITATION)) {
                bones[i].has_limit = true;
            }
			reader->Read3F32(bones[i].offset_pos);
            reader->Read(&bones[i].offset_rot, sizeof(quat));
            reader->Read3F32(bones[i].limitation);
            reader->Read(&bones[i].offset, sizeof(mat4));
        }
    }

    static FSReader* MakeReader(ImportModelInfo* info) {
        char fullpath[512];
        char path[256];
        SDL_snprintf(fullpath, 512, "%s/%s", info->path, info->file);
        SDL_memset(path, 0, 256);
        FS_ReplaceSeparators(path, fullpath);
        return new FSReader(path);
    }

    void PM2Importer::ImportModel(ImportModelInfo* info) {
        FSReader* reader = MakeReader(info);
       
        PM2_Header header;
        ReadHeader(reader, &header);

        IndexType index_type = RG_INDEX_U16;
        if (RG_CHECK_FLAG(header.flags, PM2_FLAG_EXTENDED_INDICES)) {
            index_type = RG_INDEX_U32;
        }

        PM2_MeshInfo*    mesh_info    = (PM2_MeshInfo*)rg_malloc(sizeof(PM2_MeshInfo) * header.mesh_count);
        PM2_Vertex*      vertices     = (PM2_Vertex*)rg_malloc(sizeof(PM2_Vertex) * header.vertices);
        R3D_MatMeshInfo* r3d_meshinfo = (R3D_MatMeshInfo*)rg_malloc(sizeof(R3D_MatMeshInfo) * header.mesh_count);
        R3D_Vertex*      r3d_vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * header.vertices);
        void*            indices      = rg_malloc(index_type * header.indices);
        R3D_MaterialInfo* materials   = (R3D_MaterialInfo*)rg_malloc(sizeof(R3D_MaterialInfo) * header.materials);

        AABB aabb = { {10000, 10000, 10000}, {-10000, -10000, -10000} };

        ReadMaterialsV4(&header, materials, reader, info->path);

        reader->Read(mesh_info, sizeof(PM2_MeshInfo) * header.mesh_count);
        reader->Read(vertices, sizeof(PM2_Vertex) * header.vertices);
        reader->Read(indices, index_type * header.indices);

		CopyMeshData(r3d_meshinfo, mesh_info, header.mesh_count);
        SortMeshInfo(r3d_meshinfo, header.mesh_count);
        CopyGeometryData(r3d_vertices, vertices, &aabb, header.vertices);

        // Materials
        info->info.as_static->matInfo  = materials;
        info->info.as_static->matCount = header.materials;

        // Meshes
        info->info.as_static->mInfo    = r3d_meshinfo;
        info->info.as_static->mCount   = header.mesh_count;

        // Data
        info->info.as_static->vertices = r3d_vertices;
        info->info.as_static->vCount   = header.vertices;
        info->info.as_static->indices  = indices;
        info->info.as_static->iCount   = header.indices;
        info->info.as_static->iType    = index_type;

        info->info.as_static->aabb     = aabb;

        delete reader;
        rg_free(mesh_info);
        rg_free(vertices);
    }

    struct SkeletonData {
        BoneInfo*    bones;
        PM2_IKChain* ikchains;
		Uint32 bone_count;
		Uint32 ikchain_count;
	};

    void PM2Importer::ImportRiggedModel(ImportModelInfo* info) {
        FSReader* reader = MakeReader(info);

        PM2_Header header;
        ReadHeader(reader, &header);

        IndexType index_type = RG_INDEX_U16;
        if (RG_CHECK_FLAG(header.flags, PM2_FLAG_EXTENDED_INDICES)) {
            index_type = RG_INDEX_U32;
        }

        PM2_MeshInfo*    mesh_info    = (PM2_MeshInfo*)rg_malloc(sizeof(PM2_MeshInfo) * header.mesh_count);
        PM2_Vertex*      vertices     = (PM2_Vertex*)rg_malloc(sizeof(PM2_Vertex) * header.vertices);
        PM2_Weight*      weights      = (PM2_Weight*)rg_malloc(sizeof(PM2_Weight) * header.vertices);
        R3D_MatMeshInfo* r3d_meshinfo = (R3D_MatMeshInfo*)rg_malloc(sizeof(R3D_MatMeshInfo) * header.mesh_count);
        R3D_Vertex*      r3d_vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * header.vertices);
        R3D_Weight*      r3d_weights  = (R3D_Weight*)rg_malloc(sizeof(R3D_Weight) * header.vertices);
        void*            indices      = rg_malloc(index_type * header.indices);
        R3D_MaterialInfo* materials   = (R3D_MaterialInfo*)rg_malloc(sizeof(R3D_MaterialInfo) * header.materials);

        AABB aabb = { {10000, 10000, 10000}, {-10000, -10000, -10000} };

        ReadMaterialsV4(&header, materials, reader, info->path);

        reader->Read(mesh_info, sizeof(PM2_MeshInfo) * header.mesh_count);
        reader->Read(vertices, sizeof(PM2_Vertex) * header.vertices);
        reader->Read(indices, index_type * header.indices);

        CopyMeshData(r3d_meshinfo, mesh_info, header.mesh_count);
        SortMeshInfo(r3d_meshinfo, header.mesh_count);
        CopyGeometryData(r3d_vertices, vertices, &aabb, header.vertices);

        PM2_SkeletonHeader skel_header;
        ReadSkeletonHeader(reader, &skel_header);

        /////////////////////////
		/// We need this data to construct skeleton and IK chains in KinematicsModel.
        ////////////////////////
        SkeletonData* sdata = (SkeletonData*)rg_malloc(sizeof(SkeletonData));
        sdata->bones        = (BoneInfo*)rg_malloc(sizeof(BoneInfo) * skel_header.bones);
        sdata->ikchains     = (PM2_IKChain*)rg_malloc(sizeof(PM2_IKChain) * skel_header.ikchains);
        sdata->bone_count   = skel_header.bones;
        sdata->ikchain_count= skel_header.ikchains;

        reader->Read(weights, sizeof(PM2_Weight) * header.vertices);
        ReadBones(sdata->bones, skel_header.bones, reader);
        //reader->Read(, sizeof(PM2_Bone) * skel_header.bones);
        reader->Read(sdata->ikchains, sizeof(PM2_IKChain) * skel_header.ikchains);

        CopyWeightData(r3d_weights, weights, header.vertices);
        
        info->userdata = sdata;

        // Materials
        info->info.as_rigged->matInfo  = materials;
        info->info.as_rigged->matCount = header.materials;

        // Meshes
        info->info.as_rigged->mInfo    = r3d_meshinfo;
        info->info.as_rigged->mCount   = header.mesh_count;

        // Data
        info->info.as_rigged->vertices = r3d_vertices;
        info->info.as_rigged->vCount   = header.vertices;
        info->info.as_rigged->indices  = indices;
        info->info.as_rigged->iCount   = header.indices;
        info->info.as_rigged->iType    = index_type;

        info->info.as_rigged->weights  = r3d_weights;

        info->info.as_rigged->aabb     = aabb;

        delete reader;
        rg_free(mesh_info);
        rg_free(vertices);
        rg_free(weights);
    }

    KinematicsModel* PM2Importer::LoadSkeleton(ImportModelInfo* info) {
        SkeletonData* sdata = (SkeletonData*)info->userdata;

        R3DCreateBufferInfo binfo = {};
        binfo.len = sizeof(mat4) * sdata->bone_count;
        binfo.initialData = NULL;
        R3D_BoneBuffer* bone_buffer = Render::CreateBoneBuffer(&binfo);

        KinematicsModelCreateInfo mk_info = {};

        mk_info.bone_count    = sdata->bone_count;
        mk_info.bones_info    = sdata->bones;
		// TODO: IK chains
        mk_info.ik_count      = 0;
        mk_info.ik_info       = NULL;
        mk_info.buffer_handle = bone_buffer;
        mk_info.globalInv     = MAT4_IDENTITY();

        return RG_NEW(KinematicsModel)(&mk_info);
    }

#if 0
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
#endif

    void PM2Importer::FreeModelData(FreeModelInfo* data) {
        rg_free(data->info.as_static->mInfo);
        rg_free(data->info.as_static->matInfo);
        rg_free(data->info.as_static->vertices);
        rg_free(data->info.as_static->indices);
    }

    void PM2Importer::FreeRiggedModelData(FreeModelInfo* data) {
		SkeletonData* sdata = (SkeletonData*)data->userdata;
        rg_free(sdata->bones);
        rg_free(sdata->ikchains);
        rg_free(sdata);
        rg_free(data->info.as_rigged->mInfo);
        rg_free(data->info.as_rigged->matInfo);
        rg_free(data->info.as_rigged->vertices);
        rg_free(data->info.as_rigged->indices);
        rg_free(data->info.as_rigged->weights);
    }

}