#ifndef _WINDOW_H
#define _WINDOW_H

#include "rgtypes.h"
#include "rgvector.h"

struct ImGuiContext;

namespace Engine {

    void Window_SetIcon(String p);
    void Window_SetLogo(String p);

    void Window_Initialize(String lib_renderer);
    void Window_Destroy();
    void Window_Show();
    void Window_Update();

    RG_DECLSPEC void ToggleFullscreen();
    RG_DECLSPEC bool IsFullscreen();

    RG_DECLSPEC SDL_Window* GetWindow();

    RG_DECLSPEC void GetWindowSize(ivec2* size);
    RG_DECLSPEC void SetFpsLimit(Sint32 fps);
    RG_DECLSPEC SDL_Surface* GetIconSurface();

    RG_DECLSPEC ImGuiContext* GetImGuiContext();

}

#endif