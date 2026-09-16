#ifndef _MESHTOOL_H
#define _MESHTOOL_H

#include "rgtypes.h"
#include "rendertypes.h"

namespace Engine {

	typedef struct TangentCalculateInfo {
		Uint32 vCount;
		R3D_Vertex* vertices;
		Uint32 startidx;
		Uint32 iCount;
		Uint32* indices;
		Uint32 idxtype;
	} TangentCalculateInfo;

	typedef struct NormalCalculateInfo {
		R3D_Vertex* vertices;
		void* indices;
		size_t idx_count;
		Uint32 idxtype;
	} NormalCalculateInfo;

	RG_DECLSPEC void RecalculateNormals(NormalCalculateInfo* info);
	RG_DECLSPEC void RecalculateTangetns(TangentCalculateInfo* info);

}

#endif