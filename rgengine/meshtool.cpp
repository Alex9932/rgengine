#define DLL_EXPORT
#include "meshtool.h"

namespace Engine {

	static RG_INLINE void FetchTriangle(NormalCalculateInfo* info, Uint32 i, R3D_Vertex** v0, R3D_Vertex** v1, R3D_Vertex** v2) {
		Uint32 v0idx = 0;
		Uint32 v1idx = 0;
		Uint32 v2idx = 0;

		if (info->idxtype == 2) {
			Uint16* i16 = (Uint16*)info->indices;
			v0idx = i16[i * 3 + 0];
			v1idx = i16[i * 3 + 1];
			v2idx = i16[i * 3 + 2];
		}
		else if (info->idxtype == 4 || info->idxtype == 0) { // type 0 is for compatibility
			Uint32* i32 = (Uint32*)info->indices;
			v0idx = i32[i * 3 + 0];
			v1idx = i32[i * 3 + 1];
			v2idx = i32[i * 3 + 2];
		}

		*v0 = &info->vertices[v0idx];
		*v1 = &info->vertices[v1idx];
		*v2 = &info->vertices[v2idx];
	}

	void RecalculateNormals(NormalCalculateInfo* info) {
	//void RecalculateNormals(R3D_Vertex* vertices, Uint32* indices, size_t idx_count) {
		for (Uint32 i = 0; i < info->idx_count / 3; i++) {
			R3D_Vertex* v0, * v1, * v2;
			FetchTriangle(info, i, &v0, &v1, &v2);

			vec3 v_0 = v1->pos - v0->pos;
			vec3 v_1 = v2->pos - v0->pos;

			vec3 N = v_0.cross(v_1).normalize();

			v0->norm = N;
			v1->norm = N;
			v2->norm = N;

		}
	}

	void RecalculateTangetns(TangentCalculateInfo* info) {
		// Calculate tangents
		for (Uint32 i = 0; i < info->iCount / 3; i += 3) {
#if 1
			Uint32 v0idx = 0;
			Uint32 v1idx = 0;
			Uint32 v2idx = 0;
			if (info->idxtype == 2) {
				Uint16* i16 = (Uint16*)info->indices;
				v0idx = i16[info->startidx + i + 0];
				v1idx = i16[info->startidx + i + 1];
				v2idx = i16[info->startidx + i + 2];
			}
			else if (info->idxtype == 4 || info->idxtype == 0) { // type 0 is for compatibility
				Uint32* i32 = (Uint32*)info->indices;
				v0idx = i32[info->startidx + i + 0];
				v1idx = i32[info->startidx + i + 1];
				v2idx = i32[info->startidx + i + 2];
			}
#elif
			Uint32 v0idx = info->indices[info->startidx + i + 0];
			Uint32 v1idx = info->indices[info->startidx + i + 1];
			Uint32 v2idx = info->indices[info->startidx + i + 2];
#endif
			R3D_Vertex* v0 = &info->vertices[v0idx];
			R3D_Vertex* v1 = &info->vertices[v1idx];
			R3D_Vertex* v2 = &info->vertices[v2idx];
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

	}


}