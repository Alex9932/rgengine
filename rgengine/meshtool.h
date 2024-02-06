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
	} TangentCalculateInfo;

	void RecalculateTangetns(TangentCalculateInfo* info);

}

#endif