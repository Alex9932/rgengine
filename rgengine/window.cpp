#define DLL_EXPORT
#include "window.h"
#include "timer.h"
#include "rgstb.h"
#include "engine.h"
#include "render.h"

#define RG_WND_ICON "platform/icon.png"
#define RG_WND_LOGO "platform/logo.png"

#ifdef RG_PLATFORM_WINDOWS
//#define WINDOWS_ICON
#endif

#ifdef WINDOWS_ICON
#include <SDL2/SDL_syswm.h>
#include <WinUser.h>
#endif

struct Surface {
    SDL_Surface* surface;
    Uint8* data_ptr;
};

namespace Engine {

    // Default screen resolution
    static const Uint32 w_init_width  = 1600;
    static const Uint32 w_init_height = 900;

    static Engine::Timer timer;
    static Surface icn_surface;
    static SDL_Window* hwnd_init   = NULL;
    static SDL_Window* hwnd        = NULL;
    static Uint32 w_current_width  = 0;
    static Uint32 w_current_height = 0;
    static Sint32 limit            = 0; // Set 0 to disable frame-rate limiter
    static Bool w_fullscreen       = false;

    static Surface _LoadSurfaceFromFile(String path) {
        Surface surface;
        int w, h, c;
        surface.data_ptr = RG_STB_load_from_file(path, &w, &h, &c, 4);
        surface.surface = SDL_CreateRGBSurfaceFrom(surface.data_ptr, w, h, 32, 4 * w, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        return surface;
    }

    static void _FreeSurface(Surface surface) {
        SDL_FreeSurface(surface.surface);
        RG_STB_image_free(surface.data_ptr);
    }

    void Window_Initialize(String lib_renderer) {
        icn_surface = _LoadSurfaceFromFile(RG_WND_ICON);

        timer.Update();
        if (!IsDebug()) {
            Surface bmp = _LoadSurfaceFromFile(RG_WND_LOGO);

            hwnd_init = SDL_CreateWindow("rgEngine - init", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 300, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
            hwnd = hwnd_init; // Sets error window's icon before calling Window::Show()
            SDL_SetWindowIcon(hwnd_init, icn_surface.surface);
            SDL_Renderer* ren = SDL_CreateRenderer(hwnd_init, -1, SDL_RENDERER_ACCELERATED);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, bmp.surface);

            _FreeSurface(bmp);
            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, tex, NULL, NULL);
            SDL_RenderPresent(ren);
            SDL_DestroyTexture(tex);
            SDL_DestroyRenderer(ren);
        }

        Render::LoadRenderer(lib_renderer);
        Render::Setup();

    }

    void Window_Destroy() {
        Render::DestroySubSystem();
        Render::Destroy();
        Render::UnloadRenderer();
        SDL_DestroyWindow(hwnd);
        _FreeSurface(icn_surface);
    }

    void Window_Show() {
        SDL_Delay(500);
        w_current_width = w_init_width;
        w_current_height = w_init_height;
        hwnd = Render::ShowWindow(w_current_width, w_current_height);
        SDL_assert(hwnd);
        SDL_Delay(500);

        SDL_SetWindowResizable(hwnd, SDL_TRUE);

        Render::Initialize(hwnd);
        Render::InitSubSystem();

        SDL_SetWindowIcon(hwnd, icn_surface.surface);

#ifdef WINDOWS_ICON
        HINSTANCE handle = ::GetModuleHandle(nullptr);
        HICON icon = ::LoadIconA(handle, "IDB_PNG1");
        if (icon != nullptr) {
            SDL_SysWMinfo wminfo;
            SDL_VERSION(&wminfo.version);
            if (SDL_GetWindowWMInfo(hwnd, &wminfo) == 1) {
                HWND win_hwnd = wminfo.info.win.window;
                //::SetClassLongA(win_hwnd, GCL_HICON, reinterpret_cast<LONG>(icon));
            }
        }
#endif

        SDL_Delay(500);
        if (!Engine::IsDebug()) {
            SDL_DestroyWindow(hwnd_init);
        }

    }

    void Window_Update() {
        Render::SwapBuffers();
        int w, h;
        SDL_GetWindowSize(hwnd, &w, &h);
        w_current_width = w;
        w_current_height = h;

        if (limit > 0) {
            double etime = timer.GetElapsedTime();
            double frame_time = 1.0 / (double)limit;
            if (etime < frame_time) {
                double delta = frame_time - etime;
                SDL_Delay((Uint32)(delta * 1000));
            }
        }

        timer.Update();
    }

    void ToggleFullscreen() {
        w_fullscreen = !w_fullscreen;
        if (w_fullscreen) {
            SDL_DisplayMode DM;
            SDL_GetDesktopDisplayMode(0, &DM);
            w_current_width = DM.w;
            w_current_height = DM.h;
        }
        else {
            w_current_width = w_init_width;
            w_current_height = w_init_height;
        }

        SDL_SetWindowSize(hwnd, w_current_width, w_current_height);
        SDL_SetWindowFullscreen(hwnd, w_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    }

    bool IsFullscreen() {
        return w_fullscreen;
    }

    SDL_Window* GetWindow() {
        return hwnd;
    }

    void GetWindowSize(vec2* size) {
        //size->x = w_current_width;
        //size->y = w_current_height;
        int w, h;
        SDL_GetWindowSize(hwnd, &w, &h);
        size->x = (float)w;
        size->y = (float)h;
    }

    void SetFpsLimit(Sint32 fps) {
        limit = fps;
    }

    SDL_Surface* GetIconSurface() {
        return icn_surface.surface;
    }
}