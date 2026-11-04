/*
 * rgEngine engine.cpp
 *
 *  Created on: Feb 9, 2022
 *      Author: alex9932
 */

#define DLL_EXPORT

#define RG_PRINTFPS 1

#define RG_WND_ICON "platform/icon.png"
#define RG_WND_LOGO "platform/logo.png"

#include "engine.h"
#include "rgstring.h"
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>

#include "logger.h"
#include "allocator.h"
#include "filesystem.h"
#include "profiler.h"
#include "event.h"
#include "input.h"
#include "window.h"
#include "render.h"
#include "timer.h"
#include "world.h"
#include "rgphysics.h"

#include "rgthread.h"

#include "soundsystem.h"
#include "modelsystem.h"
#include "lightsystem.h"

#include "filedialog.h"
#include "scriptengine.h"

typedef void (*PFN_MODULE_INITIALIZE)();
typedef void (*PFN_MODULE_DESTROY)();
typedef BaseGame* (*PFN_MODULE_GETAPPLICATION)();

typedef struct GameModule {
    LibraryHandle             hmodule;
    PFN_MODULE_INITIALIZE     ModuleInitialize;
    PFN_MODULE_DESTROY        ModuleDestroy;
    PFN_MODULE_GETAPPLICATION ModuleGetApplication;
} GameModule;

typedef struct EngineDiagnostics {
    String*       arg_strs      = NULL;
    Float64       uptime        = 0.0;
    Float64       acc           = 0.0;
    Sint32        arg_count     = 0;
    Uint32        num_threads   = 1;
    Uint32        clamps        = 0;
    Uint32        frame         = 0;
    Uint32        tick          = 0;
    Uint32        fps           = 0;
    Uint32        tps           = 0;
    Sint32        signal        = 0;
    Bool          is_debug      = false;
    Bool          running       = false;
    Bool          shutdown_rq   = false;
    Bool          num_tcustom   = false;
    Bool          has_signal    = false;
};

namespace Engine {

    static String        rg_assert_message = NULL;
    void SetAssertMessage(String str) { rg_assert_message = str; }

    static const Uint32  RG_SDL_INIT_CLIENT_FLAGS =
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_HAPTIC |
        SDL_INIT_GAMEPAD |
        SDL_INIT_SENSOR;

    static char          version_str[64];
    static char          platform_str[64];

    static String        fsjson        = NULL;

    static String        lib_renderer  = NULL;
    static String        lib_game      = NULL;

    static GameModule    game_module   = {};
    static EngineDiagnostics diagnostics = {};

    static BaseGame*     game_ptr      = NULL;

    static STDAllocator* std_allocator = NULL;
    static Profiler*     core_profiler = NULL;


    static Float64       frame_time    = 0.0;

    static Uint32        tickrate      = 64;
    static const int     MAX_STEPS     = 5;
    static Float64       time_scale    = 1.0;

    static Timer         timer;

    static World*        world         = NULL;

    static RGPhysics*    rgphysics     = NULL;

    static ModelSystem*  modelSystem   = NULL;
    static LightSystem*  lightSystem   = NULL;
    static SoundSystem*  soundsystem   = NULL;


    static SDL_AssertState AssertionHandler(const SDL_AssertData* data, void* userdata) {
        char message[2048];
        SDL_snprintf(message, 2048, "Assertion failure at %s (%s:%d) '%s'\n%s", data->function, data->filename, data->linenum, data->condition, rg_assert_message);
        rgLogError(RG_LOG_ASSERT, "%s", message);

        if (game_ptr->IsGraphics()) {
            // TODO
            //Error(message);
        }

        return SDL_ASSERTION_BREAK;
    }

    static bool _EventHandler(SDL_Event* event) {
        if (event->type == SDL_EVENT_QUIT) {
            Quit();
        }

        if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode== SDL_SCANCODE_F11) {
            ToggleFullscreen();
        }

        if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_F4 && IsKeyDown(SDL_SCANCODE_LSHIFT)) {
            RG_ERROR_MSG("FORCE STOP");
        }

        if (event->type == GetUserEventID() && event->user.code == RG_EVENT_SHUTDOWN_RQ) {
            rgLogInfo(RG_LOG_SYSTEM, "Shutdown request detected!");
            diagnostics.running = false;
        }

		// Temporary disabled (SDL3 migration)
#if 0
        if (event->type == SDL_EVENT_WINDOW) {
            switch (event->type) {
            case SDL_WINDOWEVENT_LEAVE: { SetFpsLimit(-1); break; }
            case SDL_WINDOWEVENT_ENTER: { SetFpsLimit(-1); break; }
            default: { break; }
            }
        }
#endif
        return true;
    }

    ////////// PUBLIC API //////////

    LibraryHandle DL_LoadLibrary(String name) {
        LibraryHandle handle;
#if defined(RG_PLATFORM_WINDOWS)
        handle = LoadLibraryA(name);
#elif defined(RG_PLATFORM_LINUX)
        handle = dlopen(name, RTLD_LAZY);
#endif
        if (handle == NULL) {
            char err_msg[128];
#if defined(RG_PLATFORM_WINDOWS)
            DWORD err_code = GetLastError();
            char errbuf[256];
            DWORD size = FormatMessageA(
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                err_code,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                errbuf,
                sizeof(errbuf),
                NULL
            );
            if (size > 0) {
				// remove trailing new line characters
                if (size >= 2 && errbuf[size - 2] == '\r') {
                    errbuf[size - 2] = '\0';
				}
                if (size >= 1 && errbuf[size - 1] == '\n') {
                    errbuf[size - 1] = '\0';
                }
            }
            SDL_snprintf(err_msg, 128, "Unable to load library: %s!\n%s (0x%.8x)", name, errbuf, err_code);
#elif defined(RG_PLATFORM_LINUX)
            SDL_snprintf(err_msg, 128, "Unable to load library: %s!", name);
#endif
            RG_ERROR_MSG(err_msg);
        }

        rgLogInfo(RG_LOG_SYSTEM, "Loaded library: %s", name);
        return handle;
    }

    ProcHandle DL_GetProcAddress(LibraryHandle handle, String proc_name) {
        ProcHandle p_handle;
#if defined(RG_PLATFORM_WINDOWS)
        p_handle = GetProcAddress(handle, proc_name);
#elif defined(RG_PLATFORM_LINUX)
        p_handle = dlsym(handle, proc_name);
#endif

        if (p_handle == NULL) {
            char err[128];
            SDL_snprintf(err, 128, "Procedure %s not found!", proc_name);
            RG_ERROR_MSG(err);
        }
        return p_handle;
    }

    void DL_UnloadLibrary(LibraryHandle handle) {
#if defined(RG_PLATFORM_WINDOWS)
        FreeLibrary(handle);
#elif defined(RG_PLATFORM_LINUX)
        dlclose(handle);
#endif
    }

    Sint32 ProcessArguments(int argc, String* argv) {
        diagnostics.arg_count = argc;
        diagnostics.arg_strs = argv;

        for (int i = 0; i < diagnostics.arg_count; ++i) {
            if (rg_streql(diagnostics.arg_strs[i], "-debug")) {
                diagnostics.is_debug = true;
            }
            else if (rg_streql(diagnostics.arg_strs[i], "-render")) {
                if (diagnostics.arg_count <= i + 1) {
                    return -1;
                }
                lib_renderer = diagnostics.arg_strs[i + 1];
            }
            else if (rg_streql(diagnostics.arg_strs[i], "-game")) {
                if (diagnostics.arg_count <= i + 1) {
                    return -1;
                }
                lib_game = diagnostics.arg_strs[i + 1];
            }
            else if (rg_streql(diagnostics.arg_strs[i], "-fsjson")) {
                if (diagnostics.arg_count <= i + 1) {
                    return -1;
                }
                fsjson = diagnostics.arg_strs[i + 1];
            }
            else if (rg_streql(diagnostics.arg_strs[i], "-t")) {
                if (diagnostics.arg_count <= i + 1) {
                    return -1;
                }
                diagnostics.num_threads = SDL_atoi(diagnostics.arg_strs[i + 1]);
                diagnostics.num_tcustom = true;
            }
        }
        return 0;
    }

    Bool IsArgument(String arg) {
        for (int i = 0; i < diagnostics.arg_count; ++i) {
            if (rg_streql(diagnostics.arg_strs[i], arg)) {
                return true;
            }
        }
        return false;
    }

    static void SignalHandler(int sig) {
        diagnostics.has_signal = true;
        diagnostics.signal     = sig;
    }

    static void ErrorSignalHandler(int sig) {
		// No expensive operations here
        // TODO: just exit
    }

    static void SetupSignalHandler() {
#if defined(RG_PLATFORM_WINDOWS)
        signal(SIGINT,   SignalHandler);
        signal(SIGTERM,  SignalHandler);
        signal(SIGBREAK, SignalHandler);
        signal(SIGSEGV,  ErrorSignalHandler);
        signal(SIGILL,   ErrorSignalHandler);
        signal(SIGFPE,   ErrorSignalHandler);
        signal(SIGABRT,  ErrorSignalHandler);
#elif defined(RG_PLATFORM_LINUX)

#endif
    }

    static void rgCPUID(char* CPUBrandString) {
        int CPUInfo[4] = { -1 };
        unsigned nExIds, i = 0;
        __cpuid(CPUInfo, 0x80000000);
        nExIds = CPUInfo[0];
        for (i = 0x80000000; i <= nExIds; ++i) {
            __cpuid(CPUInfo, i);
            if (i == 0x80000002) {
                memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
            }
            else if (i == 0x80000003) {
                memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
            }
            else if (i == 0x80000004) {
                memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
            }
        }
    }

    static void SetupGameModule() {
        rgLogInfo(RG_LOG_SYSTEM, "Loading game module...");
        if (lib_game == NULL) { RG_ERROR_MSG("No game! No life!"); }
        
        game_module.hmodule = DL_LoadLibrary(lib_game);
        game_module.ModuleInitialize     = (PFN_MODULE_INITIALIZE)DL_GetProcAddress(game_module.hmodule, "Module_Initialize");
        game_module.ModuleDestroy        = (PFN_MODULE_DESTROY)DL_GetProcAddress(game_module.hmodule, "Module_Destroy");
        game_module.ModuleGetApplication = (PFN_MODULE_GETAPPLICATION)DL_GetProcAddress(game_module.hmodule, "Module_GetApplication");

        rgLogInfo(RG_LOG_SYSTEM, "Initializing game module...");
        game_module.ModuleInitialize();
        game_ptr = game_module.ModuleGetApplication();

        String icon = RG_WND_ICON;
        String logo = RG_WND_LOGO;
        if (game_ptr->wndIcon) { icon = game_ptr->wndIcon; }
        if (game_ptr->wndLogo) { logo = game_ptr->wndLogo; }

        Window_SetIcon(icon);
        Window_SetLogo(logo);

        if (!game_ptr) { RG_ERROR_MSG("Invalid handle"); }
        rgLogInfo(RG_LOG_SYSTEM, "Game: %s", game_ptr->GetName());
    }

    static void DestroyGameModule() {
        rgLogInfo(RG_LOG_SYSTEM, "Unloading game module...");
        game_module.ModuleDestroy();
        DL_UnloadLibrary(game_module.hmodule);
    }

    void Initialize() {
        diagnostics.running = true;

        SDL_Init(SDL_INIT_EVENTS);
        SetupSignalHandler();


        SDL_snprintf(version_str, 64, "%d.%d.%d", RG_VERSION_MAJ, RG_VERSION_MIN, RG_VERSION_PATCH);
        SDL_snprintf(platform_str, 64, "%s", SDL_GetPlatform());

        Logger_Initialize();

        rgLogInfo(RG_LOG_SYSTEM, "Initializing engine...");
        SDL_SetAssertionHandler(AssertionHandler, NULL);

        int ver = SDL_GetVersion();

        rgLogInfo(RG_LOG_SYSTEM, "Engine version: %d.%d.%d", RG_VERSION_MAJ, RG_VERSION_MIN, RG_VERSION_PATCH);
        rgLogInfo(RG_LOG_SYSTEM, "SDL version: %d.%d.%d", SDL_VERSIONNUM_MAJOR(ver), SDL_VERSIONNUM_MINOR(ver), SDL_VERSIONNUM_MICRO(ver));

        char CPUBrandString[64] = {};
        rgCPUID(CPUBrandString);

        rgLogInfo(RG_LOG_SYSTEM, "Platform: %s, %s %s CPU, line %d, %d Mb ram.",
            SDL_GetPlatform(),
            CPUBrandString,
            GetCpuNameStr(SDL_GetNumLogicalCPUCores()), //SDL_GetCPUCount()
            SDL_GetCPUCacheLineSize(),
            SDL_GetSystemRAM());
        rgLogInfo(RG_LOG_SYSTEM, "Using log path: %s", Logger_GetLogPath());

        if (diagnostics.is_debug) {
            rgLogWarn(RG_LOG_SYSTEM, " ~ ");
            rgLogWarn(RG_LOG_SYSTEM, " ~ Engine started in DEBUG profile!");
            rgLogWarn(RG_LOG_SYSTEM, " ~ ");
        }

        SetupGameModule();

        std_allocator = new STDAllocator("SYSTEM");
        //RegisterAllocator(std_allocator);
        SetDefaultAllocator(std_allocator);
        core_profiler = RG_NEW_CLASS(std_allocator, Profiler)();

        Filesystem_Initialize(fsjson);

        if (game_ptr->IsGraphics()) {
            if (!SDL_Init(SDL_INIT_VIDEO) || !SDL_Init(SDL_INIT_AUDIO)) {
                rgLogError(RG_LOG_SYSTEM, "SDL: %s", SDL_GetError());
            }

            if (lib_renderer == NULL) {
                RG_ERROR_MSG("No renderer!");
            }

            Window_Initialize(lib_renderer);
        }
        if (!game_ptr->IsClient()) {
            Input_StartConsole();
        }

        Event_Initialize();
        Input_Initialize();

        RegisterEventHandler(_EventHandler);

        rgphysics = RG_NEW_CLASS(std_allocator, RGPhysics)();
        world = RG_NEW_CLASS(GetDefaultAllocator(), World)();

        //Commands_Initialize();
        //Cvars_Initialize();

        if (!diagnostics.num_tcustom) {
            diagnostics.num_threads = SDL_GetNumLogicalCPUCores();
        }
        Thread_Initialize(diagnostics.num_threads);

        Script_Initialize();

        //Network_Initialize();

        if (game_ptr->IsClient()) {
            soundsystem = RG_NEW_CLASS(std_allocator, SoundSystem)();
            RG_NFDInit();
        }

        if (game_ptr->IsGraphics()) {
            Window_Show();
            modelSystem = RG_NEW_CLASS(std_allocator, ModelSystem)();
            lightSystem = RG_NEW_CLASS(std_allocator, LightSystem)();
        }

        game_ptr->Initialize();
    }

    static String profiles[] = {
        "input",
        "tick",
        "present",
        "threads",
        "swapbuffer",
        "other"
    };

    String GetProfile(Uint32 idx) {
        if (idx < 6) { return profiles[idx]; }
        return "null";
    }

    void Start() {
        rgLogInfo(RG_LOG_SYSTEM, "Starting engine...");
        PushEvent(0, RG_EVENT_STANDBY, NULL, NULL);

        timer.Update();

        double last_time = timer.GetTime();
        Uint32 frame = 0;
        Uint32 tick = 0;

        while (diagnostics.running) {

            Float64 step = 1.0 / (Float64)tickrate;

            core_profiler->Reset();
            frame_time = timer.GetElapsedTime();
            timer.Update();
            diagnostics.uptime += frame_time;

            core_profiler->StartSection(profiles[0]);
            HandleEvents();
            UpdateInput();

            core_profiler->StartSection(profiles[1]);
            diagnostics.acc += min(frame_time, 0.25) * time_scale;
            Uint32 steps = 0;
            while (diagnostics.acc >= step && steps < MAX_STEPS) {
                world->BeginTick();
                world->AdvanceStep(step);
                rgphysics->StepSimulation(step);
                world->ApplyQueued();
                game_ptr->FixedUpdate(step);

                tick++;
                steps++;
                diagnostics.acc -= step;
                diagnostics.tick++;
            }
            if (diagnostics.acc >= step) { diagnostics.acc = 0; diagnostics.clamps++; } // Overload, skip ticks

            Float64 alpha = diagnostics.acc / step;

            core_profiler->StartSection(profiles[2]);
            world->PresentUpdate(frame_time, alpha);
            game_ptr->MainUpdate(frame_time, alpha);

            if (game_ptr->IsClient()) {
                soundsystem->Update(frame_time);
            }

            if (game_ptr->IsGraphics()) {
                modelSystem->UpdateComponents();
                lightSystem->UpdateComponents();

                Render::UpdateSystems();
            }

            core_profiler->StartSection(profiles[3]);
            Thread_Execute();
            Thread_WaitForAll();

            core_profiler->StartSection(profiles[4]);
            if (game_ptr->IsGraphics()) {
                Render::Update();
                Window_Update();
            }

            core_profiler->StartSection(profiles[5]);
            
            if (diagnostics.has_signal) {
                switch (diagnostics.signal) {
                    case SIGINT:   { rgLogWarn(RG_LOG_SYSTEM, "SIGNAL: Interrupt"); break; }
                    case SIGTERM:  { rgLogWarn(RG_LOG_SYSTEM, "SIGNAL: Terminate"); break; }
                    case SIGBREAK: { rgLogWarn(RG_LOG_SYSTEM, "SIGNAL: Break"); break; }
                    default:       { rgLogWarn(RG_LOG_SYSTEM, "SIGNAL: Terminate (unknown)"); break; }
                }
                rgLogWarn(RG_LOG_SYSTEM, "Requesting shutdown...\n");
                Quit();
            }

            diagnostics.frame++;
            frame++;

            static Float64 _input = 0;
            static Float64 _tick = 0;
            static Float64 _present = 0;
            static Float64 _threads = 0;
            static Float64 _swap = 0;

#if 1
			if(timer.GetTime() - last_time >= 5.0) {
#else
            if (timer.GetTime() - last_time >= 1.0) {
#endif

#if RG_PRINTFPS
#if 1
				rgLogInfo(RG_LOG_SYSTEM, "FPS: %d, TPS: %d", frame / 5, tick / 5);
#else
                rgLogInfo(RG_LOG_SYSTEM, "[avg] Fps: %d, tickrate: %d", frame, tick);
                rgLogInfo(RG_LOG_SYSTEM, "[current] Fps: %d, acc %lf, acc resets: %d", (Uint32)(1.0f / frame_time), acc, clamps);
                rgLogInfo(RG_LOG_SYSTEM, "Input: %lf ms, tick: %lf ms, present: %lf ms, threads: %lf ms, swap: %lf ms",
                    _input * 1000,
                    _tick * 1000,
                    _present * 1000,
                    _threads * 1000,
                    _swap * 1000);
#endif
#endif
                diagnostics.fps = frame / 5;
                diagnostics.tps = tick / 5;
                last_time = timer.GetTime();
                frame = 0;
                tick = 0;
            }


            _input += core_profiler->GetTime(profiles[0]); _input = _input / 2.0;
            _tick += core_profiler->GetTime(profiles[1]); _tick = _tick / 2.0;
            _present += core_profiler->GetTime(profiles[2]); _present = _present / 2.0;
            _threads += core_profiler->GetTime(profiles[3]); _threads = _threads / 2.0;
            _swap += core_profiler->GetTime(profiles[4]); _swap = _swap / 2.0;

        }

        // Quit
        rgLogInfo(RG_LOG_SYSTEM, "Stopping engine...");
        game_ptr->Quit();

        if (game_ptr->IsClient()) {
            RG_NFDDestroy();
            RG_DELETE_CLASS(std_allocator, SoundSystem, soundsystem);
        }

        // Save imgui path
        char imguipath[512];
        SDL_strlcpy(imguipath, GetGame()->imguiIni, 512);
        DestroyGameModule();

        Script_Destroy();

        Thread_Destroy();

        RG_DELETE_CLASS(GetDefaultAllocator(), World, world);
        RG_DELETE_CLASS(GetDefaultAllocator(), RGPhysics, rgphysics);

        if (game_ptr->IsGraphics()) {
            RG_DELETE_CLASS(std_allocator, ModelSystem, modelSystem);
            RG_DELETE_CLASS(std_allocator, LightSystem, lightSystem);
            Window_Destroy(imguipath);
        }

        Input_Destroy();
        Event_Destroy();

        Filesystem_Destroy();
        RG_DELETE_CLASS(std_allocator, Profiler, core_profiler);
        //FreeAllocator(std_allocator);
        delete std_allocator;
        Logger_Destroy();
        SDL_Quit();
    }

    void RequestShutdown() {
        
        int btn = 0;
        SDL_MessageBoxButtonData boxbtns[2] = {};
        SDL_MessageBoxColorScheme boxcolor = {};
        SDL_MessageBoxData boxdata = {};

        boxbtns[0].buttonID = 1;
        boxbtns[0].flags = 0;
        boxbtns[0].text = "Yes";
        boxbtns[1].buttonID = 2;
        boxbtns[1].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
        boxbtns[1].text = "No";

        // TODO: Create GLOBAL color scheme
        boxcolor.colors[SDL_MESSAGEBOX_COLOR_BACKGROUND] = { 25, 25, 25 };
        boxcolor.colors[SDL_MESSAGEBOX_COLOR_TEXT] = { 205, 205, 205 };
        boxcolor.colors[SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] = { 0, 0, 0 };
        boxcolor.colors[SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] = { 32, 32, 32 };
        boxcolor.colors[SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] = { 55, 55, 55 };

        boxdata.flags       = SDL_MESSAGEBOX_WARNING;
        boxdata.window      = GetWindow();
        boxdata.title       = "rgEngine";
        boxdata.message     = "Do you want quit?";
        boxdata.numbuttons  = 2;
        boxdata.buttons     = boxbtns;
        boxdata.colorScheme = &boxcolor;

        SDL_ShowMessageBox(&boxdata, &btn);
        if (btn == 1) {
            Quit();
        }
    }

    void Quit() {
        diagnostics.shutdown_rq = true;
        PushEvent(0, RG_EVENT_SHUTDOWN_RQ, NULL, NULL);
    }

    void ForceQuit() {
        rgLogWarn(RG_LOG_SYSTEM, "Engine: FORCE QUIT!");
        Logger_Destroy();
        // TODO: Add macro for this operation
#if defined(RG_PLATFORM_WINDOWS)
        ExitProcess(0xFFFFFFFF);
#else
        exit(-1);
#endif
    }

    void HandleError(String message) {
		// TODO: Lock for first caller thread. Block other threads until the engine is terminated.

        String quotes_err[] = {
            "Try turning it off and on. Then read the log.",
            "Not a total disaster. Just a partial one.",
            "Some things broke. Endmin is disappointed.",
            "Errors happened. Blame the dolphins.",
            "Chen refuses to comment on this outcome.",
            "Gilberta got lost on the way. Rare, but it happens.",
            "This is fine. Probably.",
			"The engine is on fire. Please call the fire department.",
            "Undefined behavior. Very defined consequences.",
            "Congratulations! You found an edge case.",
            "Everything was working in the previous frame.",
            "The engine has encountered an unexpected situation.",
            "Something went wrong. Details are in the log.",
            "The operation was successful. The result was not.",
            "The engine is no longer accepting reasonable requests.",
            "Something somewhere stopped making sense.",
            "Well, that was unexpected.",
            "Maybe the next build will be better.",
            "Somebody forgot to check the return value."
        };

        size_t poolSize = sizeof(quotes_err) / sizeof(quotes_err[0]);
        Uint64 uptime_bits = 0;
        SDL_memcpy(&uptime_bits, &diagnostics.uptime, sizeof(Uint64));
        size_t rndidx = uptime_bits % poolSize;

        rgLogError(RG_LOG_SYSTEM, "");
        rgLogError(RG_LOG_SYSTEM, " * * * * * * * * * ENGINE ERROR * * * * * * * * *\n");
        rgLogError(RG_LOG_SYSTEM, "");
        rgLogError(RG_LOG_SYSTEM, " ~ %s", quotes_err[rndidx]);
        rgLogError(RG_LOG_SYSTEM, "");
        rgLogError(RG_LOG_SYSTEM, "[ENGINE]");
        rgLogError(RG_LOG_SYSTEM, "  IsDebug: %u", diagnostics.is_debug);
        rgLogError(RG_LOG_SYSTEM, "  Uptime: %lf seconds", diagnostics.uptime);
        rgLogError(RG_LOG_SYSTEM, "  Error: %s", message);
        rgLogError(RG_LOG_SYSTEM, "  Threads: %u, Running: %u, stop request: %u", diagnostics.num_threads, diagnostics.running, diagnostics.shutdown_rq);
        if (lib_game) {
            rgLogError(RG_LOG_SYSTEM, "  Game: %s", lib_game);
        }
        else {
            rgLogError(RG_LOG_SYSTEM, "  Game: Not present");
        }
        rgLogError(RG_LOG_SYSTEM, "  Commandline[%d]:", diagnostics.arg_count);
        for (Sint32 i = 0; i < diagnostics.arg_count; i++) {
            rgLogError(RG_LOG_SYSTEM, "   %s", diagnostics.arg_strs[i]);
        }

        rgLogError(RG_LOG_SYSTEM, "");
        rgLogError(RG_LOG_SYSTEM, "[RENDER]");
        if (lib_renderer) {
            rgLogError(RG_LOG_SYSTEM, "  Backend: %s", lib_renderer);
            RenderInfo renderer_info = {};
            Render::GetInfo(&renderer_info);

            rgLogError(RG_LOG_SYSTEM, "  Renderer: %s", renderer_info.renderer);
            rgLogError(RG_LOG_SYSTEM, "  API: %s", renderer_info.render_name);
            rgLogError(RG_LOG_SYSTEM, "  GPU memory: %llu Kb", renderer_info.dedicated_memory >> 10);
            rgLogError(RG_LOG_SYSTEM, "  Shared memory: %llu Kb", renderer_info.shared_memory >> 10);
            rgLogError(RG_LOG_SYSTEM, "  Buffers memory: %llu Kb", renderer_info.buffers_memory >> 10);
            rgLogError(RG_LOG_SYSTEM, "  Textures memory: %llu Kb", renderer_info.textures_memory >> 10);
        }
        else {
            rgLogError(RG_LOG_SYSTEM, "  Not present");
        }

        rgLogError(RG_LOG_SYSTEM, "");
        rgLogError(RG_LOG_SYSTEM, "[FRAME]");
        rgLogError(RG_LOG_SYSTEM, "  FPS/Frame: %u/%u\n", diagnostics.fps, diagnostics.frame);
        rgLogError(RG_LOG_SYSTEM, "");
        rgLogError(RG_LOG_SYSTEM, "[SIMULATION]");
        rgLogError(RG_LOG_SYSTEM, "  TPS/Tick: %u/%u\n", diagnostics.tps, diagnostics.tick);
        rgLogError(RG_LOG_SYSTEM, "  Acc/clamps: %lf/%u", diagnostics.acc, diagnostics.clamps);

        //SDL_ShowSimpleMessageBox(0, "rgEngine fatal error", message, GetWindow());
        ForceQuit();
    }

    Bool IsDebug() {
        return diagnostics.is_debug;
    }

    Bool IsRunning() {
        return diagnostics.running;
    }

    Float64 GetDeltaTime() {
        return frame_time;
    }

    Float64 GetUptime() {
        return diagnostics.uptime;
    }

    Uint32 GetThreads() {
        return diagnostics.num_threads;
    }

    BaseGame* GetGame() {
        return game_ptr;
    }

    String GetEngineVersion() {
        return version_str;
    }

    String GetEnginePlatform() {
        return platform_str;
    }

    World* GetWorld() {
        return world;
    }

    SoundSystem* GetSoundSystem() {
        return soundsystem;
    }

    ModelSystem* GetModelSystem() {
        return modelSystem;
    }

    LightSystem* GetLightSystem() {
        return lightSystem;
    }

    Profiler* GetProfiler() {
        return core_profiler;
    }

    RGPhysics* GetPhysics() {
        return rgphysics;
    }

}