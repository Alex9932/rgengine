#define DLL_EXPORT

#include "scriptengine.h"
#include "scriptruntime.h"
#include <mujs.h>
#include "allocator.h"
#include "filesystem.h"

namespace Engine {

	static STDAllocator* alloc;

	static js_Report JS_DefaultReport;

	void Script_Initialize() {
		alloc = RG_NEW_CLASS(GetDefaultAllocator(), STDAllocator)("ScriptAlloc");
	}

	void Script_Destroy() {
		RG_DELETE_CLASS(GetDefaultAllocator(), STDAllocator, alloc);
	}

	static void ReportProc(js_State* J, const char* message) {
		rgLogWarn(RG_LOG_SCRIPT, "%s", message);
	}

	static void PCall(RGScriptState* state, String str) {
		if (js_pcall(state->J, 0)) {
			rgLogError(RG_LOG_SCRIPT, "EE Script runtime error %s:%s", str, js_tostring(state->J, -1));
			js_pop(state->J, 1);
			return;
		}
	}

	RGScriptState* Script_MakeState() {
		RGScriptState* state = (RGScriptState*)alloc->Allocate(sizeof(RGScriptState));
		state->J = js_newstate(NULL, NULL, JS_STRICT);
		js_setreport(state->J, ReportProc);
		// TODO: Add custom functions
		SetupBaseRuntime(state);
		return state;
	}

	void Script_FreeState(RGScriptState* state) {
		js_freestate(state->J);
		alloc->Deallocate(state);
	}

	void Script_LoadCode(RGScriptState* state, String source, String file) {
		if (js_ploadstring(state->J, file, source)) {
			rgLogError(RG_LOG_SCRIPT, "Script error: %s:%s", file, js_tostring(state->J, -1));
			js_pop(state->J, 1);
			return;
		}
		// Call loaded function
		js_pushundefined(state->J);
		PCall(state, file);
		js_pop(state->J, 1);
	}

	void Script_CallFunction(RGScriptState* state, String name) {
		rgLogInfo(RG_LOG_SCRIPT, "Script execute: %s", name);
		js_getglobal(state->J, name);

		if (!js_iscallable(state->J, -1)) {
			rgLogError(RG_LOG_SCRIPT, "Function error(%s is %s): no executable code was found!", name, js_tostring(state->J, -1));
			js_pop(state->J, 1);
			return;
		}

		js_pushundefined(state->J);
		PCall(state, name);
		js_pop(state->J, 1);
	}

	void Script_ExecuteFile(RGScriptState* state, String file) {
		Resource* res = GetResource(file);
		void* data = alloc->Allocate(res->length + 1);
		SDL_memset(data, 0, res->length + 1);
		SDL_memcpy(data, res->data, res->length);
		Script_LoadCode(state, (String)data, file);
		Script_CallFunction(state, "main");
		alloc->Deallocate(data);
		FreeResource(res);
	}

}