// dllmain.cpp : Defines the entry point for the DLL application.
#if 0
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#endif

#include "rgtypes.h"

BOOL APIENTRY DllMain(HMODULE hmod, DWORD  ul_reason_for_call, LPVOID lpReserved) {
#if 0
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    char name[128];
    GetModuleFileNameA(hmod, name, 128);
    rgLogWarn(RG_LOG_SYSTEM, "Engine DLL: %s -> %d", name, ul_reason_for_call);
#endif
    return TRUE;
}
