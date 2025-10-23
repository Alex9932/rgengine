#define DLL_EXPORT
#include "window.h"
#include "timer.h"
#include "rgstb.h"
#include "engine.h"
#include "render.h"

#include "event.h"
#include "filesystem.h"

#ifdef RG_PLATFORM_WINDOWS
//#define WINDOWS_ICON
#endif

#ifdef WINDOWS_ICON
#include <SDL2/SDL_syswm.h>
#include <WinUser.h>
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"

struct Surface {
    SDL_Surface* surface;
    Uint8* data_ptr;
};

namespace Engine {

    // Default screen resolution
    static const Uint32  w_init_width     = 1600;
    static const Uint32  w_init_height    = 900;

    static Engine::Timer timer;
    static Surface       icn_surface;
    static SDL_Window*   hwnd_init        = NULL;
    static SDL_Window*   hwnd             = NULL;
    static Uint32        w_current_width  = 0;
    static Uint32        w_current_height = 0;
    static Sint32        limit            = 0; // Set 0 to disable frame-rate limiter
    static Bool          w_fullscreen     = false;

    static ImGuiContext* imctx            = NULL;
    
    static char          WND_ICON[128]    = {};
    static char          WND_LOGO[128]    = {};

    static Surface _LoadSurfaceFromFile(String path) {
        Surface surface;
        int w, h, c;
        surface.data_ptr = RG_STB_load_from_file(path, &w, &h, &c, 4);
        //surface.surface = SDL_CreateRGBSurfaceFrom(surface.data_ptr, w, h, 32, 4 * w, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        surface.surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_ABGR8888, surface.data_ptr, w*4);
        return surface;
    }

    static void _FreeSurface(Surface surface) {
        SDL_DestroySurface(surface.surface);
        RG_STB_image_free(surface.data_ptr);
    }

    static Bool _EventHandler(SDL_Event* event) {
        ImGui_ImplSDL3_ProcessEvent(event);
        return true;
    }

    void Window_SetIcon(String p) {
        SDL_snprintf(WND_ICON, 128, p);
    }

    void Window_SetLogo(String p) {
        SDL_snprintf(WND_LOGO, 128, p);
    }

    void Window_Initialize(String lib_renderer) {
        icn_surface = _LoadSurfaceFromFile(WND_ICON);

        timer.Update();
        if (!IsDebug()) {
            Surface bmp = _LoadSurfaceFromFile(WND_LOGO);

            //hwnd_init = SDL_CreateWindow("rgEngine - init", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 300, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
            hwnd_init = SDL_CreateWindow("rgEngine - init", 600, 300, SDL_WINDOW_BORDERLESS);
            SDL_SetWindowPosition(hwnd_init, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            hwnd = hwnd_init; // Sets error window's icon before calling Window::Show()
            SDL_SetWindowIcon(hwnd_init, icn_surface.surface);
            //SDL_Renderer* ren = SDL_CreateRenderer(hwnd_init, -1, SDL_RENDERER_ACCELERATED);
            SDL_Renderer* ren = SDL_CreateRenderer(hwnd_init, NULL);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, bmp.surface);

            _FreeSurface(bmp);
            SDL_RenderClear(ren);
            SDL_RenderTexture(ren, tex, NULL, NULL);
            SDL_RenderPresent(ren);
            SDL_DestroyTexture(tex);
            SDL_DestroyRenderer(ren);
        }

        Render::LoadRenderer(lib_renderer);

    }

    void Window_Destroy() {

        Render::DestroySubSystem();
        Render::UnloadRenderer();

        FreeEventHandler(_EventHandler);

        // Save ImGui state
        char imcfgpath[512];
        GetPath(imcfgpath, 512, RG_PATH_USERDATA, GetGame()->imguiIni);
        size_t imini_len = 0;
        String imini = ImGui::SaveIniSettingsToMemory(&imini_len);
        FSWriter writer(imcfgpath);
		writer.Write(imini, imini_len);
        writer.Flush();
        

        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(imctx);

        SDL_DestroyWindow(hwnd);
        _FreeSurface(icn_surface);
    }

    static void SetupImGuiStyle() {
        // Classic Steam style by metasprite from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.6000000238418579f;
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 1.0f;
        style.WindowMinSize = ImVec2(32.0f, 32.0f);
        style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding = 0.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 0.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(4.0f, 3.0f);
        style.FrameRounding = 0.0f;
        style.FrameBorderSize = 1.0f;
        style.ItemSpacing = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.CellPadding = ImVec2(4.0f, 2.0f);
        style.IndentSpacing = 21.0f;
        style.ColumnsMinSpacing = 6.0f;
        style.ScrollbarSize = 14.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabMinSize = 10.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;
        style.TabBorderSize = 0.0f;
        //style.TabMinWidthForCloseButton = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.2862745225429535f, 0.3372549116611481f, 0.2588235437870026f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.2862745225429535f, 0.3372549116611481f, 0.2588235437870026f, 1.0f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.239215686917305f, 0.2666666805744171f, 0.2000000029802322f, 1.0f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.5372549295425415f, 0.5686274766921997f, 0.5098039507865906f, 0.5f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.1372549086809158f, 0.1568627506494522f, 0.1098039224743843f, 0.5199999809265137f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.239215686917305f, 0.2666666805744171f, 0.2000000029802322f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2666666805744171f, 0.2980392277240753f, 0.2274509817361832f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2980392277240753f, 0.3372549116611481f, 0.2588235437870026f, 1.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.239215686917305f, 0.2666666805744171f, 0.2000000029802322f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2862745225429535f, 0.3372549116611481f, 0.2588235437870026f, 1.0f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.239215686917305f, 0.2666666805744171f, 0.2000000029802322f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.3490196168422699f, 0.4196078479290009f, 0.3098039329051971f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.2784313857555389f, 0.3176470696926117f, 0.239215686917305f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.2470588237047195f, 0.2980392277240753f, 0.2196078449487686f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.2274509817361832f, 0.2666666805744171f, 0.2078431397676468f, 1.0f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.5882353186607361f, 0.5372549295425415f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.3490196168422699f, 0.4196078479290009f, 0.3098039329051971f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5372549295425415f, 0.5686274766921997f, 0.5098039507865906f, 0.5f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.2862745225429535f, 0.3372549116611481f, 0.2588235437870026f, 0.4000000059604645f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3490196168422699f, 0.4196078479290009f, 0.3098039329051971f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.5372549295425415f, 0.5686274766921997f, 0.5098039507865906f, 0.5f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.3490196168422699f, 0.4196078479290009f, 0.3098039329051971f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3490196168422699f, 0.4196078479290009f, 0.3098039329051971f, 0.6000000238418579f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.5372549295425415f, 0.5686274766921997f, 0.5098039507865906f, 0.5f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.1372549086809158f, 0.1568627506494522f, 0.1098039224743843f, 1.0f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.5372549295425415f, 0.5686274766921997f, 0.5098039507865906f, 1.0f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.5882353186607361f, 0.5372549295425415f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1882352977991104f, 0.2274509817361832f, 0.1764705926179886f, 0.0f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.5372549295425415f, 0.5686274766921997f, 0.5098039507865906f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.5882353186607361f, 0.5372549295425415f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.3490196168422699f, 0.4196078479290009f, 0.3098039329051971f, 1.0f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.5372549295425415f, 0.5686274766921997f, 0.5098039507865906f, 0.7799999713897705f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.5882353186607361f, 0.5372549295425415f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.239215686917305f, 0.2666666805744171f, 0.2000000029802322f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.3490196168422699f, 0.4196078479290009f, 0.3098039329051971f, 1.0f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.5882353186607361f, 0.5372549295425415f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.7764706015586853f, 0.2784313857555389f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.5882353186607361f, 0.5372549295425415f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.729411780834198f, 0.6666666865348816f, 0.239215686917305f, 1.0f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.5882353186607361f, 0.5372549295425415f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
    }

    void Window_Show() {
        SDL_Delay(500);
        w_current_width = w_init_width;
        w_current_height = w_init_height;
        hwnd = Render::ShowWindow(w_current_width, w_current_height);
        SDL_assert(hwnd);
        SDL_Delay(500);

        // ImGui
        imctx = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplSDL3_InitForOther(hwnd);

        SetupImGuiStyle();
        //ImGui::StyleColorsDark();
        
        // Load ImGui config
        char imcfgpath[512];
        GetPath(imcfgpath, 512, RG_PATH_USERDATA, GetGame()->imguiIni);
        Resource* res = GetResource(imcfgpath);
        if (res) {
            ImGui::LoadIniSettingsFromMemory((String)res->data, res->length);
            FreeResource(res);
        }

        RegisterEventHandler(_EventHandler);

        ImGui_ImplSDL3_NewFrame();


        SDL_SetWindowResizable(hwnd, true);

        Render::InitializeContext(hwnd);
        Render::InitSubSystem();

        SDL_SetWindowIcon(hwnd, icn_surface.surface);

        ImGui::NewFrame();

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
        // End frame
        ImGui::EndFrame();
        ImGui::Render();

        //ImGuiIO& io = ImGui::GetIO();
        //io.DisplaySize.x = w_current_width;
        //io.DisplaySize.y = w_current_height;

        // Begin new frame & swap buffers
        ImGui_ImplSDL3_NewFrame();
        Render::SwapBuffers();
        ImGui::NewFrame();

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
            const SDL_DisplayMode* DM;
            //SDL_GetDesktopDisplayMode(0, &DM);
            DM = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(hwnd));
            w_current_width = DM->w;
            w_current_height = DM->h;
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

    void GetWindowSize(ivec2* size) {
        SDL_GetWindowSize(hwnd, &size->x, &size->y);
    }

    void SetFpsLimit(Sint32 fps) {
        limit = fps;
    }

    SDL_Surface* GetIconSurface() {
        return icn_surface.surface;
    }

    ImGuiContext* GetImGuiContext() {
        return imctx;
    }
}