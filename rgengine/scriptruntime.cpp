#include "scriptruntime.h"
#include "rgtypes.h"

namespace Engine {

	static void rt_print(js_State* J) {
		rgLogInfo(RG_LOG_SYSTEM, "[script] %s\n", js_tostring(J, 1));
		js_pushundefined(J);
	}

	void SetupBaseRuntime(RGScriptState* state) {
		js_newcfunction(state->J, rt_print, "print", 1);
		js_setglobal(state->J, "print");
	}

}