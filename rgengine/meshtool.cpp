#include "meshtool.h"

namespace Engine {

	void RecalculateTangetns(TangentCalculateInfo* info) {
		// Calculate tangents
		for (Uint32 i = 0; i < info->iCount / 3; i += 3) {
			Uint32 v0idx = info->indices[info->startidx + i + 0];
			Uint32 v1idx = info->indices[info->startidx + i + 1];
			Uint32 v2idx = info->indices[info->startidx + i + 2];
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