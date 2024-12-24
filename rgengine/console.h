#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "rgtypes.h"

namespace Engine {

    // Private
	void InitializeConsole();
	void DestroyConsole();
    void UpdateConsole();

    // Public
    RG_DECLSPEC void ShowConsole();
    RG_DECLSPEC void HideConsole();
    RG_DECLSPEC void ToggleConsole();
    RG_DECLSPEC Bool IsConsoleShown();

}

#endif