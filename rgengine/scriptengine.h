#ifndef _SCRIPTENGINE_H
#define _SCRIPTENGINE_H

#include "rgtypes.h"

struct RGScriptState;

namespace Engine {

	void Script_Initialize();
	void Script_Destroy();

	RG_DECLSPEC RGScriptState* Script_MakeState();
	RG_DECLSPEC void Script_FreeState(RGScriptState* state);

	RG_DECLSPEC void Script_LoadCode(RGScriptState* state, String source, String file = "inline");
	RG_DECLSPEC void Script_ExecuteFile(RGScriptState* state, String file);

}

#endif