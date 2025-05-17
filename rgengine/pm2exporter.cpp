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

// [Materials] v2/v3 (Outdated)
// {
//  PM2_String albedo;
//  PM2_String normal;
//  PM2_String pbr;    // v3
//  vec4       color;
// }
// 
// [Materials] v4
// {
//  PM2_String texture; ( may be w/o normal map !!! albedo & pbr REQUIRED !!!)
//  vec3       color;
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

// Skeleton extension

// [Skeleton header]
// {
//  char   sig[4];    "PM2S"
//  Uint32 bone_count;
//  Uint32 ikchain_count;
// }

// [Weights]
// {
//  PM2_Vec4  weights;
//  PM2_IVec4 boneids;
// }

// [Bone]
// {
//  PM2_String name;
//  Sint32     parent;
//  Uint32     type;
//  PM2_Vec3   position;
// }

// [Skeleton]
// {
//	Bone[] bones;
// }

// [IKChain]
// {
//  Uint32   target;
//  Uint32   effector;
//  Uint32   iterations;
//  Float32  angle_limit;
//  Uint32   bones;
//  Uint32[] list;
// }

// [IK]
// {
//	IKChain[] chains;
// }

// Pbysics extension ( FOR FUTURE USE )

// [Rigid bodies]
// ...


namespace Engine {

	static void WritePM2String(FSWriter* writer, String str) {
		Uint32 len = SDL_strlen(str);
		//writer->WriteU32(len);
		writer->Write(&len, 4);
		writer->Write(str, len);
	}

	static RG_INLINE Uint32 GetIndex(IndexType type, void* indices, Uint32 idx) {
		if (type == RG_INDEX_U32) {
			Uint32* inds = (Uint32*)indices;
			return inds[idx];
		} else if (type == RG_INDEX_U16) {
			Uint16* inds = (Uint16*)indices;
			return inds[idx];
		} else if (type == RG_INDEX_U8) {
			Uint8* inds = (Uint8*)indices;
			return inds[idx];
		} else {
			return 0;
		}
	}

	static FSWriter* MakeWriter(String p) {
		char path[256];
		SDL_memset(path, 0, 256);
		FS_ReplaceSeparators(path, p);

		FSWriter* writer = new FSWriter(path);

		return writer;
	}

	static void WriteMaterials(FSWriter* writer, Uint32 count, R3D_MaterialInfo* mats) {
		for (Uint32 i = 0; i < count; i++) {
			WritePM2String(writer, mats[i].texture);
			writer->Write(&mats[i].color, sizeof(vec3));
		}
	}

	static void WriteMeshes(FSWriter* writer, Uint32 count, R3D_MatMeshInfo* meshes) {
		for (Uint32 i = 0; i < count; i++) {
			//writer->WriteU32(info->mInfo[i].materialIdx);
			//writer->WriteU32(info->mInfo[i].indexCount);
			// ???
			writer->Write(&meshes[i].materialIdx, 4);
			writer->Write(&meshes[i].indexCount, 4);
		}
	}

	static void WriteVertexData(FSWriter* writer, Uint32 count, R3D_Vertex* vertices, mat4* matrix) {
		mat4 modelmatrix = MAT4_IDENTITY();
		if (matrix) {
			modelmatrix = *matrix;
		}

		PM2_Vertex* dst_vertices = (PM2_Vertex*)rg_malloc(sizeof(PM2_Vertex) * count);
		for (Uint32 i = 0; i < count; i++) {
			vec4 pos4;
			vec4 norm4;
			vec4 tang4;
			pos4.xyz  = vertices[i].pos;  pos4.w  = 1;
			norm4.xyz = vertices[i].norm; norm4.w = 0;
			tang4.xyz = vertices[i].tang; tang4.w = 0;

			vec4 npos  = modelmatrix * pos4;
			vec4 nnorm = modelmatrix * norm4;
			vec4 ntang = modelmatrix * tang4;

			dst_vertices[i].position = npos.xyz;
			dst_vertices[i].normal   = nnorm.xyz;
			dst_vertices[i].tangent  = ntang.xyz;

			dst_vertices[i].uv = vertices[i].uv;
		}

		writer->Write(dst_vertices, sizeof(PM2_Vertex) * count);
		rg_free(dst_vertices);
	}

	static void WriteIndexData(FSWriter* writer, Uint32 count, void* indices, IndexType type) {
		Uint32 indexsize = 2;
		if (count > 0xFFFF) { indexsize = 4; }
		void* dst_indices = rg_malloc(indexsize * count);

		if (indexsize == 2) {
			Uint16* indicesptr = (Uint16*)dst_indices;
			for (Uint32 i = 0; i < count; i++) {
				indicesptr[i] = GetIndex(type, indices, i);
			}
		}
		else if (indexsize == 4) {
			Uint32* indicesptr = (Uint32*)dst_indices;
			for (Uint32 i = 0; i < count; i++) {
				indicesptr[i] = GetIndex(type, indices, i);
			}
		}

		writer->Write(dst_indices, indexsize * count);
		rg_free(dst_indices);
	}

	static void WriteBones(FSWriter* writer, Uint32 count, Bone* bones) {
		Uint32 i_null = 0;

		for (Uint32 i = 0; i < count; i++) {
			WritePM2String(writer, bones[i].name);
			writer->Write(&bones[i].parent, 4);
			writer->Write(&i_null, 4); // bone flags
			writer->Write(&bones[i].offset_pos, sizeof(vec3));
		}
	}

	static void WriteIKChains(FSWriter* writer, Uint32 count, IKList* chains) {
		for (Uint32 i = 0; i < count; i++) {
			writer->Write(&chains[i].target, 4);
			writer->Write(&chains[i].effector, 4);
			writer->Write(&chains[i].iterations, 4);
			writer->Write(&chains[i].angle_limit, 4);
			writer->Write(&chains[i].bones, 4);
			for (Uint32 k = 0; k < chains[i].bones; k++) {
				writer->Write(&chains[i].list[k], 4);
			}
		}
	}

	template<typename T>
	static void WriteBaseModel(FSWriter* writer, T* info, mat4* model, Uint8 flags) {
		PM2_Header header = {};
		header.sig[0]    = 'P'; header.sig[1] = 'M'; header.sig[2] = '2'; header.sig[3] = ' ';
		header.materials = info->matCount;
		header.vertices  = info->vCount;
		header.indices   = info->iCount;
		header.flags     = flags;
		header.version   = 4;
		header.offset    = info->mCount;
		writer->Write(&header, sizeof(PM2_Header));

		WriteMaterials(writer, info->matCount, info->matInfo);
		WriteMeshes(writer, info->mCount, info->mInfo);
		WriteVertexData(writer, info->vCount, info->vertices, model);
		WriteIndexData(writer, info->iCount, info->indices, info->iType);
	}

	void PM2Exporter::ExportModel(String p, R3DStaticModelInfo* info, mat4* model) {

		FSWriter* writer = MakeWriter(p);

		Uint8 flags = 0;
		if (info->vCount > 0xFFFF) {
			flags |= PM2_FLAG_EXTENDED_INDICES;
		}

		WriteBaseModel(writer, info, model, flags);

		delete writer;
	}

	void PM2Exporter::ExportRiggedModel(String p, R3DRiggedModelInfo* info, KinematicsModel* kmdl, mat4* model) {

		FSWriter* writer = MakeWriter(p);

		Uint8 flags = PM2_FLAG_SKELETON;
		if (info->vCount > 0xFFFF) {
			flags |= PM2_FLAG_EXTENDED_INDICES;
		}

		WriteBaseModel(writer, info, model, flags);

		// Write skeleton info
		PM2_SkeletonHeader sheader = {};
		sheader.sig[0]   = 'P'; sheader.sig[1] = 'M'; sheader.sig[2] = '2'; sheader.sig[3] = 'S';
		sheader.bones    = kmdl->GetBoneCount();
		sheader.ikchains = kmdl->GetIKListCount();
		writer->Write(&sheader, sizeof(PM2_SkeletonHeader));

		writer->Write(info->weights, sizeof(R3D_Weight) * info->vCount);
		WriteBones(writer, kmdl->GetBoneCount(), kmdl->GetBones());
		WriteIKChains(writer, kmdl->GetIKListCount(), kmdl->GetIKLists());

		delete writer;
	}

}