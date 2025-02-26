#ifndef _SCRIPTRUNTIME_H
#define _SCRIPTRUNTIME_H

#include <mujs.h>

typedef struct RGScriptState {
	js_State* J;
} RGScriptState;

namespace Engine {

	void SetupBaseRuntime(RGScriptState* state);

}

#endif