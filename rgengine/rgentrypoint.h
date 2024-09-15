#ifndef _ENTRYPOINT_H
#define _ENTRYPOINT_H

#include "engine.h"

#undef main
#define rgmain(fptr)                               \
int main(int argc, char** argv) {                  \
	Engine::ProcessArguments(argc, (String*)argv); \
	return fptr(argc, (String*)argv);              \
}

#ifdef GAME_DLL
#ifdef __cplusplus
extern "C" {
#endif
	extern RG_DECLSPEC void Module_Initialize();
	extern RG_DECLSPEC void Module_Destroy();
	extern RG_DECLSPEC BaseGame* Module_GetApplication();
#ifdef __cplusplus
	}
#endif
#endif

#endif