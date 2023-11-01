#if 0

#define DLL_EXPORT
#include "pm2.h"

#include "filesystem.h"
#include "engine.h"
#include "render.h"

R3D_StaticModel* PM2_LoadModel(String path) {
    Engine::FSReader* reader = new Engine::FSReader(path);

    char model_root[256];
    Engine::FS_PathFrom(model_root, path, 256);

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

    if (header.version != 2) {
        RG_ERROR_MSG("Unsupported pm2 version!");
    }

    IndexType index_type = RG_INDEX_U16;
    if (RG_CHECK_FLAG(header.flags, PM2_FLAG_EXTENDED_INDICES)) {
        index_type = RG_INDEX_U32;
    }

    PM2_V2_MeshInfo* mesh_info = (PM2_V2_MeshInfo*)rg_malloc(sizeof(PM2_V2_MeshInfo) * header.offset); // offset - mesh count in pm2 v2
    PM2_Vertex* vertices = (PM2_Vertex*)rg_malloc(sizeof(PM2_Vertex) * header.vertices);

    R3D_MeshInfo* r3d_meshinfo = (R3D_MeshInfo*)rg_malloc(sizeof(R3D_MeshInfo) * header.offset);
    R3D_Vertex* r3d_vertices = (R3D_Vertex*)rg_malloc(sizeof(R3D_Vertex) * header.vertices);
    void* indices = rg_malloc(index_type * header.indices);

    //Uint32 materials[header.materials];
    R3D_Material** materials = (R3D_Material**)rg_malloc(sizeof(R3D_Material*) * header.materials);

    char d_str_buffer[256];
    char n_str_buffer[256];
    char p_str_buffer[256];
    char str_buffer[256];

    for (Uint32 i = 0; i < header.materials; i++) {
        SDL_memset(d_str_buffer, 0, 256);
        SDL_memset(n_str_buffer, 0, 256);

        Uint32 len = reader->ReadU32();
        SDL_memset(str_buffer, 0, 256);
        reader->Read(str_buffer, len);
        SDL_snprintf(d_str_buffer, 256, "%s/%s", model_root, str_buffer);
        len = reader->ReadU32();
        SDL_memset(str_buffer, 0, 256);
        reader->Read(str_buffer, len);
        SDL_snprintf(n_str_buffer, 256, "%s/%s", model_root, str_buffer);

        Engine::GetPath(p_str_buffer, 256, RG_PATH_SYSTEM, "textures/def_pbr2.png");

        vec4 diffuse;
        reader->Read4F32(diffuse);
#if 0
        R_Image* img_albedo = Render::MakeImage(d_str_buffer);
        R_Image* img_normal = Render::MakeImage(n_str_buffer);
        R_Image* img_pbr = Render::MakeImage(p_str_buffer);
#endif
        R3DCreateMaterialInfo mat_info = {};
#if 0
        mat_info.albedo = img_albedo;
        mat_info.normal = img_normal;
        mat_info.pbr = img_pbr;
#endif
        mat_info.albedo = d_str_buffer;
        mat_info.normal = n_str_buffer;
        mat_info.pbr    = p_str_buffer;
        mat_info.color = { diffuse.r, diffuse.g, diffuse.b };
        //mat_info.shininess = 8;
        //mat_info.flags = R3D_MATERIAL_CULL_BACK_FACE;
        materials[i] = Engine::Render::R3D_CreateMaterial(&mat_info);
    }

    reader->Read(mesh_info, sizeof(PM2_V2_MeshInfo) * header.offset);
    reader->Read(vertices, sizeof(PM2_Vertex) * header.vertices);
    reader->Read(indices, index_type * header.indices);

    // To R3D_Model
    for (Uint32 i = 0; i < header.offset; i++) {
        r3d_meshinfo[i].indexCount = mesh_info[i].indices;
        r3d_meshinfo[i].material   = materials[mesh_info[i].material];
    }

    for (Uint32 i = 0; i < header.vertices; i++) {
        r3d_vertices[i].pos.x = vertices[i].position.x;
        r3d_vertices[i].pos.y = vertices[i].position.y;
        r3d_vertices[i].pos.z = vertices[i].position.z;
        r3d_vertices[i].norm.x = vertices[i].normal.x;
        r3d_vertices[i].norm.y = vertices[i].normal.y;
        r3d_vertices[i].norm.z = vertices[i].normal.z;
        r3d_vertices[i].tang.x = vertices[i].tangent.x;
        r3d_vertices[i].tang.y = vertices[i].tangent.y;
        r3d_vertices[i].tang.z = vertices[i].tangent.z;
        r3d_vertices[i].uv.x = vertices[i].uv.x;
        r3d_vertices[i].uv.y = vertices[i].uv.y;
    }

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

    R3DCreateStaticModelInfo info = {};
    info.info     = r3d_meshinfo;
    info.vertices = r3d_vertices;
    info.indices  = indices;
    info.vCount   = header.vertices;
    info.iCount   = header.indices;
    info.mCount   = header.offset;
    info.iType    = index_type;

    rg_free(mesh_info);
    rg_free(vertices);
    rg_free(materials);
    delete reader;

    R3D_StaticModel* mdl = Engine::Render::R3D_CreateStaticModel(&info);

    rg_free(r3d_meshinfo);
    rg_free(r3d_vertices);
    rg_free(indices);


    return mdl;
}

#endif