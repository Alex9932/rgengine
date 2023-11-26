/*
 * rgEngine engine.cpp
 *
 *  Created on: Feb 9, 2022
 *      Author: alex9932
 */

#define DLL_EXPORT

#include "engine.h"
#include "rgstring.h"
#include <stdio.h>

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

void* rg_malloc(size_t size) {
    if (size > 0x7FFFFFFF) {
        RG_ERROR_MSG("OUT OF MEMORY!");
        return NULL;
    }
    return malloc(size);
}

void rg_free(void* ptr) {
    free(ptr);
}

namespace Engine {

    static String        rg_assert_message = NULL;
    void SetAssertMessage(String str) { rg_assert_message = str; }

    static const Uint32  RG_SDL_INIT_CLIENT_FLAGS =
        SDL_INIT_AUDIO |
        SDL_INIT_VIDEO |
        SDL_INIT_JOYSTICK |
        SDL_INIT_HAPTIC |
        SDL_INIT_GAMECONTROLLER |
        SDL_INIT_SENSOR;

    static char          version_str[64];
    static char          platform_str[64];

    static int           arg_count     = 0;
    static String*       arg_strs      = NULL;

    static Bool          is_debug      = false;
    static String        fsjson        = NULL;
    static int           num_threads   = 1;
    static String        lib_renderer  = NULL;

    static BaseGame*     game_ptr      = NULL;
    static Bool          running       = false;

    static STDAllocator* std_allocator = NULL;
    static Profiler*     core_profiler = NULL;

    static Bool          shutdown_rq   = false;

    static Float64       frame_time    = 0.0;
    static Float64       uptime        = 0.0;

    static Timer         timer;

    static World*        world         = NULL;


    static SDL_AssertState AssertionHandler(const SDL_AssertData* data, void* userdata) {
        rgLogError(RG_LOG_ASSERT, "Assertion failure at %s (%s:%d) '%s' %s", data->function, data->filename, data->linenum, data->condition, rg_assert_message);

        if (game_ptr->IsGraphics()) {
            char message[2048];
            SDL_snprintf(message, 2048, "Assertion failure at %s (%s:%d) '%s'\n%s", data->function, data->filename, data->linenum, data->condition, rg_assert_message);
            // TODO
            //Error(message);
        }

        return SDL_ASSERTION_BREAK;
    }

    static bool _EventHandler(SDL_Event* event) {
        if (event->type == SDL_QUIT) {
            Quit();
        }

        if (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_F11) {
            ToggleFullscreen();
        }

        if (event->type == GetUserEventID() && event->user.code == RG_EVENT_SHUTDOWN_RQ) {
            rgLogInfo(RG_LOG_SYSTEM, "Shutdown request detected!");
            running = false;
        }

        if (event->type == SDL_WINDOWEVENT) {
            switch (event->window.event) {
                case SDL_WINDOWEVENT_LEAVE: { SetFpsLimit(-1); break; }
                case SDL_WINDOWEVENT_ENTER: { SetFpsLimit(-1); break; }
                default: { break; }
            }
        }

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
            char err[128];
            SDL_snprintf(err, 128, "Unable to load library: %s!", name);
            RG_ERROR_MSG(err);
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
        arg_count = argc;
        arg_strs  = argv;

        for (int i = 0; i < arg_count; ++i) {
            if (rg_streql(arg_strs[i], "-debug")) {
                is_debug = true;
            } else if (rg_streql(arg_strs[i], "-render")) {
                if (arg_count <= i + 1) {
                    return -1;
                }
                lib_renderer = arg_strs[i + 1];
            } else if (rg_streql(arg_strs[i], "-fsjson")) {
                if (arg_count <= i + 1) {
                    return -1;
                }
                fsjson = arg_strs[i + 1];
            } else if (rg_streql(arg_strs[i], "-t")) {
                if (arg_count <= i + 1) {
                    return -1;
                }
                num_threads = SDL_atoi(arg_strs[i + 1]);
            }
        }
        return 0;
    }

    Bool IsArgument(String arg) {
        for (int i = 0; i < arg_count; ++i) {
            if (rg_streql(arg_strs[i], arg)) {
                return true;
            }
        }
        return false;
    }

    static void handle_signal(int sig) {
        printf("SIGNAL: Terminate\n");

        switch (sig) {
            case SIGINT: {
                break;
            }
            case SIGILL: {
                break;
            }
            case SIGFPE: {
                break;
            }
            case SIGSEGV: {
                printf("SIGNAL: Segmentation violation\n");
                break;
            }
            case SIGTERM: {
                break;
            }
            case SIGBREAK: {
                break;
            }
            case SIGABRT: {
                break;
            }
            default: {
                break;
            }
        }
    }

    void Initialize(BaseGame* game) {
        game_ptr = game;
        running  = true;

        SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS);

#if defined(RG_PLATFORM_WINDOWS)
        signal(SIGINT,   handle_signal);
        signal(SIGILL,   handle_signal);
        signal(SIGFPE,   handle_signal);
        signal(SIGSEGV,  handle_signal);
        signal(SIGTERM,  handle_signal);
        signal(SIGBREAK, handle_signal);
        signal(SIGABRT,  handle_signal);
#elif defined(RG_PLATFORM_LINUX)

#endif

        SDL_snprintf(version_str, 64, "%d.%d.%d", RG_VERSION_MAJ, RG_VERSION_MIN, RG_VERSION_PATCH);
        SDL_snprintf(platform_str, 64, "%s", SDL_GetPlatform());

        Logger_Initialize();

        rgLogInfo(RG_LOG_SYSTEM, "Initializing engine...");
        SDL_SetAssertionHandler(AssertionHandler, NULL);

        SDL_version ver;
        SDL_GetVersion(&ver);

        rgLogInfo(RG_LOG_SYSTEM, "Engine version: %d.%d.%d, Build: %d", RG_VERSION_MAJ, RG_VERSION_MIN, RG_VERSION_PATCH, RG_BUILD);
        rgLogInfo(RG_LOG_SYSTEM, "SDL version: %d.%d.%d", ver.major, ver.minor, ver.patch);

        int CPUInfo[4] = { -1 };
        unsigned nExIds, i = 0;
        char CPUBrandString[64] = {};
        __cpuid(CPUInfo, 0x80000000);
        nExIds = CPUInfo[0];
        for (i = 0x80000000; i <= nExIds; ++i) {
            __cpuid(CPUInfo, i);
            if (i == 0x80000002) {
                memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
            } else if (i == 0x80000003) {
                memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
            } else if (i == 0x80000004) {
                memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
            }
        }

        rgLogInfo(RG_LOG_SYSTEM, "Platform: %s, %s %s CPU, line %d, %d Mb ram.",
            SDL_GetPlatform(),
            CPUBrandString,
            GetCpuNameStr(SDL_GetCPUCount()),
            SDL_GetCPUCacheLineSize(),
            SDL_GetSystemRAM());
        rgLogInfo(RG_LOG_SYSTEM, "Using log path: %s", Logger_GetLogPath());

        if (is_debug) {
            rgLogWarn(RG_LOG_SYSTEM, " ~ ");
            rgLogWarn(RG_LOG_SYSTEM, " ~ Engine started in DEBUG profile!");
            rgLogWarn(RG_LOG_SYSTEM, " ~ ");
        }
        rgLogInfo(RG_LOG_SYSTEM, "Game: %s", game_ptr->GetName());

        std_allocator = new STDAllocator("SYSTEM");
        //RegisterAllocator(std_allocator);
        SetDefaultAllocator(std_allocator);
        core_profiler = RG_NEW_CLASS(std_allocator, Profiler)();

        Filesystem_Initialize(fsjson);

        if (game_ptr->IsClient()) {
            if (lib_renderer == NULL) {
                RG_ERROR_MSG("No renderer!");
            }

            Window_Initialize(lib_renderer);
        }

        Event_Initialize();
        Input_Initialize();

        RegisterEventHandler(_EventHandler);

        world = RG_NEW_CLASS(GetDefaultAllocator(), World)();

        //Commands_Initialize();
        //Cvars_Initialize();

        //Thread_Initialize();

        //Network_Initialize();

        if (game_ptr->IsClient()) {
            Window_Show();
            //SoundSystem_Initialize();
            //soundsystem = RG_NEW_CLASS(std_allocator, SoundSystem)();
        }

        //physicssystem = RG_NEW_CLASS(std_allocator, PhysicsSystem)();

        //world = RG_NEW_CLASS(std_allocator, World)();

        game_ptr->Initialize();
    }

    void Start() {
        rgLogInfo(RG_LOG_SYSTEM, "Starting engine...");
        PushEvent(0, RG_EVENT_STANDBY, NULL, NULL);

        timer.Update();

        double last_time = timer.GetTime();
        Uint32 frame = 0;

        while (running) {

#if 0
            Float64 input_t  = core_profiler->GetTime("input");
            Float64 game_t   = core_profiler->GetTime("game");
            Float64 render_t = core_profiler->GetTime("render");
            rgLogInfo(RG_LOG_SYSTEM, "I: %lf, G: %lf, R: %lf", input_t, game_t, render_t);
#endif

            core_profiler->Reset();
            frame_time = timer.GetElapsedTime();
            timer.Update();
            uptime += frame_time;

            core_profiler->StartSection("input");
            HandleEvents();
            UpdateInput();

            core_profiler->StartSection("worldupdate");
            world->Update();

            core_profiler->StartSection("game");
            game_ptr->MainUpdate();

            if (game_ptr->IsClient()) {
                core_profiler->StartSection("render");
                Render::Update();
                //core_profiler->StartSection("audio");
                //GetSoundSystem()->Update(GetDeltaTime());
                //soundsystem->Update(GetDeltaTime());
            }

            core_profiler->StartSection("other");
            frame++;
            //			if(timer.GetTime() - last_time >= 5.0) {
            //				rgLogInfo(RG_LOG_SYSTEM, "Fps: %d", frame / 5);
            if (timer.GetTime() - last_time >= 1.0) {
                rgLogInfo(RG_LOG_SYSTEM, "Fps: %d", frame);
                last_time = timer.GetTime();
                frame = 0;
            }

        }

        // Quit
        rgLogInfo(RG_LOG_SYSTEM, "Stopping engine...");

        game_ptr->Quit();

        //RG_DELETE_CLASS(std_allocator, World, world);

        if (game_ptr->IsClient()) {
            //RG_DELETE_CLASS(std_allocator, SoundSystem, soundsystem);
            //DestroySoundSystem();
            Window_Destroy();
        }

        // TODO

        RG_DELETE_CLASS(GetDefaultAllocator(), World, world);

        Input_Destroy();
        Event_Destroy();

        Filesystem_Destroy();
        RG_DELETE_CLASS(std_allocator, Profiler, core_profiler);
        //FreeAllocator(std_allocator);
        delete std_allocator;
        Logger_Destroy();
        SDL_Quit();
    }

    void Quit() {
        shutdown_rq = true;
        PushEvent(0, RG_EVENT_SHUTDOWN_RQ, NULL, NULL);
    }

    void ForceQuit() {
        Logger_Destroy();
#if defined(RG_PLATFORM_WINDOWS)
        ExitProcess(0xFFFFFFFF);
#else
        exit(-1);
#endif
    }

    void HandleError(String message) {
        SDL_ShowSimpleMessageBox(0, "rgEngine fatal error", message, GetWindow());
        ExitProcess(0xFFFFFFFF);
    }

    Bool IsDebug() {
        return is_debug;
    }

    Bool IsRunning() {
        return running;
    }

    Float64 GetDeltaTime() {
        return frame_time;
    }

    Float64 GetUptime() {
        return uptime;
    }

    Uint32 GetThreads() {
        return num_threads;
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

}