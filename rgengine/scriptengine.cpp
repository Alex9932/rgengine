#define DLL_EXPORT

#include "scriptengine.h"
#include "scriptruntime.h"
#include <mujs.h>
#include "allocator.h"
#include "filesystem.h"

namespace Engine {

	static STDAllocator* alloc;

	void Script_Initialize() {
		alloc = RG_NEW_CLASS(GetDefaultAllocator(), STDAllocator)("ScriptAlloc");
	}

	void Script_Destroy() {
		RG_DELETE_CLASS(GetDefaultAllocator(), STDAllocator, alloc);
	}

	RGScriptState* Script_MakeState() {
		RGScriptState* state = (RGScriptState*)alloc->Allocate(sizeof(RGScriptState));
		state->J = js_newstate(NULL, NULL, JS_STRICT);
		// TODO: Add custom functions
		SetupBaseRuntime(state);
		return state;
	}

	void Script_FreeState(RGScriptState* state) {
		js_freestate(state->J);
		alloc->Deallocate(state);
	}

	void Script_Execute(RGScriptState* state, String source) {
		js_dostring(state->J, source);
	}

	void Script_ExecuteFile(RGScriptState* state, String file) {
		Resource* res = GetResource(file);
		void* data = alloc->Allocate(res->length + 1);
		SDL_memset(data, 0, res->length + 1);
		SDL_memcpy(data, res->data, res->length);
		js_dostring(state->J, (String)data);
		alloc->Deallocate(data);
		FreeResource(res);
	}

}