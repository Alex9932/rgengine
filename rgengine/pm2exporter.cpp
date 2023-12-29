#define DLL_EXPORT
#include "pm2exporter.h"
#include "pm2.h"
#include "filesystem.h"
#include "allocator.h"

// PM2 File structure

// [Header]
//  char   sig[4];    "PM2 "
//  Uint32 materials; Materials count
//  Uint32 vertices;  Vertices/weights count
//  Uint32 indices;   Indices count
//  Uint8  flags;     Flags
//  Uint8  version;   PM2 Version
//  Uint16 offset;    Mesh count in PM2 v>=2 | 2-byte offset in PM2 v1

// [Materials] v2/v3
// {
//  PM2_String albedo;
//  PM2_String normal;
//  PM2_String pbr;    // v3
//  vec4       color;     
// }

// [Mesh info]
// {
//  Uint32 material; Material ID
//  Uint32 indices;  Indices count
// }

// [Vertices]
// {
//  PM2_Vec3 position;
//  PM2_Vec3 normal;
//  PM2_Vec3 tangent;
//  PM2_Vec2 uv;
// }

// [Indices]
// {
//  [IndexSize] idx;  16-bit index by default (32-bit index if PM2_FLAG_EXTENDED_INDICES flag set)
// }

// FUTURE (Not implemented yet)

// [Weights]
// {
//  PM2_Vec4  weights;
//  PM2_IVec4 boneids;
// }

// [Skeleton]
// ...

// [Rigid bodies]
// ...


namespace Engine {

	static void WritePM2String(FSWriter* writer, String str) {
		Uint32 len = SDL_strlen(str);
		//writer->WriteU32(len);
		writer->Write(&len, 4);
		writer->Write(str, len);
	}

	static RG_INLINE Uint32 GetIndex(R3DStaticModelInfo* info, Uint32 idx) {
		if (info->iType == RG_INDEX_U32) {
			Uint32* inds = (Uint32*)info->indices;
			return inds[idx];
		} else if (info->iType == RG_INDEX_U16) {
			Uint16* inds = (Uint16*)info->indices;
			return inds[idx];
		} else if (info->iType == RG_INDEX_U8) {
			Uint8* inds = (Uint8*)info->indices;
			return inds[idx];
		} else {
			return 0;
		}
	}

	void PM2Exporter::ExportModel(String p, R3DStaticModelInfo* info, mat4* model) {

		char path[256];
		SDL_memset(path, 0, 256);
		FS_ReplaceSeparators(path, p);

		FSWriter* writer = new FSWriter(path);

		Uint8 flags = 0;
		if (info->vCount > 0xFFFF) {
			flags |= PM2_FLAG_EXTENDED_INDICES;
		}

		PM2_Header header = {};
		header.sig[0] = 'P'; header.sig[1] = 'M'; header.sig[2] = '2'; header.sig[3] = ' ';
		header.materials = info->matCount;
		header.vertices  = info->vCount;
		header.indices   = info->iCount;
		header.flags     = flags;
		header.version   = 3;
		header.offset    = info->mCount;

		// Header
		writer->Write(&header, sizeof(PM2_Header));

		// Materials
		for (Uint32 i = 0; i < info->matCount; i++) {
			R3D_MaterialInfo* mat = &info->matInfo[i];
			WritePM2String(writer, mat->albedo);
			WritePM2String(writer, mat->normal);
			WritePM2String(writer, mat->pbr);
			vec4 color = { mat->color.x, mat->color.y, mat->color.z, 1};
			//writer->Write4F32(&color);
			writer->Write(&color, sizeof(vec4));
		}

		// Mesh info
		for (Uint32 i = 0; i < info->mCount; i++) {
			//writer->WriteU32(info->mInfo[i].materialIdx);
			//writer->WriteU32(info->mInfo[i].indexCount);
			writer->Write(&info->mInfo[i].materialIdx, 4);
			writer->Write(&info->mInfo[i].indexCount, 4);
		}

		// Vertices

		mat4 modelmatrix = MAT4_IDENTITY();
		if (model) {
			modelmatrix = *model;
		}

		PM2_Vertex* vertices = (PM2_Vertex*)rg_malloc(sizeof(PM2_Vertex) * info->vCount);
		for (Uint32 i = 0; i < info->vCount; i++) {
			vec4 pos4;
			vec4 norm4;
			vec4 tang4;
			pos4.xyz  = info->vertices[i].pos;  pos4.w  = 1;
			norm4.xyz = info->vertices[i].norm; norm4.w = 0;
			tang4.xyz = info->vertices[i].tang; tang4.w = 0;

			vec4 npos  = modelmatrix * pos4;
			vec4 nnorm = modelmatrix * norm4;
			vec4 ntang = modelmatrix * tang4;

			vertices[i].position = npos.xyz;
			vertices[i].normal   = nnorm.xyz;
			vertices[i].tangent  = ntang.xyz;

			vertices[i].uv       = info->vertices[i].uv;
		}

		writer->Write(vertices, sizeof(PM2_Vertex) * info->vCount);
		rg_free(vertices);

		// Indices
		Uint32 indexsize = 2;
		if (info->vCount > 0xFFFF) { indexsize = 4; }
		void* indices = rg_malloc(indexsize * info->iCount);

		if (indexsize == 2) {
			Uint16* indicesptr = (Uint16*)indices;
			for (Uint32 i = 0; i < info->iCount; i++) {
				indicesptr[i] = GetIndex(info, i);
			}
		} else if(indexsize == 4) {
			Uint32* indicesptr = (Uint32*)indices;
			for (Uint32 i = 0; i < info->iCount; i++) {
				indicesptr[i] = GetIndex(info, i);
			}
		}

		writer->Write(indices, indexsize * info->iCount);
		rg_free(indices);

		delete writer;
	}

}