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
#include "rgphysics.h"

#include "rgthread.h"

#include "soundsystem.h"

#include "filedialog.h"

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

typedef void (*PFN_MODULE_INITIALIZE)();
typedef void (*PFN_MODULE_DESTROY)();
typedef BaseGame* (*PFN_MODULE_GETAPPLICATION)();

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
    static Uint32        num_threads   = 1;
    static Bool          num_tcustom   = false;
    static String        lib_renderer  = NULL;

    static String        lib_game      = NULL;
    static LibraryHandle game_module   = 0;

    static BaseGame*     game_ptr      = NULL;
    static Bool          running       = false;

    static STDAllocator* std_allocator = NULL;
    static Profiler*     core_profiler = NULL;

    static Bool          shutdown_rq   = false;

    static Float64       frame_time    = 0.0;
    static Float64       uptime        = 0.0;

    static Timer         timer;

    static World*        world         = NULL;

    static RGPhysics*    rgphysics     = NULL;

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
            char err_msg[128];
            DWORD err_code = GetLastError();
            SDL_snprintf(err_msg, 128, "Unable to load library: %s!\nError code: 0x%.8x", name, err_code);
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
        arg_count = argc;
        arg_strs = argv;

        for (int i = 0; i < arg_count; ++i) {
            if (rg_streql(arg_strs[i], "-debug")) {
                is_debug = true;
            }
            else if (rg_streql(arg_strs[i], "-render")) {
                if (arg_count <= i + 1) {
                    return -1;
                }
                lib_renderer = arg_strs[i + 1];
            }
            else if (rg_streql(arg_strs[i], "-game")) {
                if (arg_count <= i + 1) {
                    return -1;
                }
                lib_game = arg_strs[i + 1];
            }
            else if (rg_streql(arg_strs[i], "-fsjson")) {
                if (arg_count <= i + 1) {
                    return -1;
                }
                fsjson = arg_strs[i + 1];
            }
            else if (rg_streql(arg_strs[i], "-t")) {
                if (arg_count <= i + 1) {
                    return -1;
                }
                num_threads = SDL_atoi(arg_strs[i + 1]);
                num_tcustom = true;
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

    static void SignalHandler(int sig) {
        switch (sig) {
            case SIGINT:   { printf("SIGNAL: Interrupt\n"); break; }
            case SIGILL:   { printf("SIGNAL: Illegal instruction\n"); break; }
            case SIGFPE:   { printf("SIGNAL: Floating point exception\n"); break; }
            case SIGSEGV:  { printf("SIGNAL: Segmentation violation\n"); break; }
            case SIGTERM:  { printf("SIGNAL: Terminate\n"); break; }
            case SIGBREAK: { printf("SIGNAL: Break\n"); Quit(); break; }
            case SIGABRT:  { printf("SIGNAL: Abnormal termination\n"); break; }
            default: { printf("SIGNAL: Terminate (unknown)\n"); break; }
        }
    }

    static void SetupSignalHandler() {
#if defined(RG_PLATFORM_WINDOWS)
        // Testing
        signal(SIGINT,   SignalHandler);
        signal(SIGILL,   SignalHandler);
        signal(SIGFPE,   SignalHandler);
        signal(SIGSEGV,  SignalHandler);
        signal(SIGTERM,  SignalHandler);
        signal(SIGBREAK, SignalHandler);
        signal(SIGABRT,  SignalHandler);
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
        PFN_MODULE_INITIALIZE     ModuleInitialize     = NULL;
        PFN_MODULE_GETAPPLICATION ModuleGetApplication = NULL;

        rgLogInfo(RG_LOG_SYSTEM, "Loading game module...");

        if (lib_game == NULL) {
            RG_ERROR_MSG("No game! No life!");
        }

        game_module = DL_LoadLibrary(lib_game);

        ModuleInitialize     = (PFN_MODULE_INITIALIZE)DL_GetProcAddress(game_module, "Module_Initialize");
        ModuleGetApplication = (PFN_MODULE_GETAPPLICATION)DL_GetProcAddress(game_module, "Module_GetApplication");

        rgLogInfo(RG_LOG_SYSTEM, "Initializing game module...");
        ModuleInitialize();
        game_ptr = ModuleGetApplication();

        if (!game_ptr) { RG_ERROR_MSG("Invalid handle"); }
        rgLogInfo(RG_LOG_SYSTEM, "Game: %s", game_ptr->GetName());
    }

    void Initialize() {
        running = true;

        SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS);
        SetupSignalHandler();


        SDL_snprintf(version_str, 64, "%d.%d.%d", RG_VERSION_MAJ, RG_VERSION_MIN, RG_VERSION_PATCH);
        SDL_snprintf(platform_str, 64, "%s", SDL_GetPlatform());

        Logger_Initialize();

        rgLogInfo(RG_LOG_SYSTEM, "Initializing engine...");
        SDL_SetAssertionHandler(AssertionHandler, NULL);

        SDL_version ver;
        SDL_GetVersion(&ver);

        rgLogInfo(RG_LOG_SYSTEM, "Engine version: %d.%d.%d, Build: %d", RG_VERSION_MAJ, RG_VERSION_MIN, RG_VERSION_PATCH, RG_BUILD);
        rgLogInfo(RG_LOG_SYSTEM, "SDL version: %d.%d.%d", ver.major, ver.minor, ver.patch);

        char CPUBrandString[64] = {};
        rgCPUID(CPUBrandString);

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

        SetupGameModule();

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

        rgphysics = RG_NEW_CLASS(std_allocator, RGPhysics)();
        world = RG_NEW_CLASS(GetDefaultAllocator(), World)();

        //Commands_Initialize();
        //Cvars_Initialize();

        if (!num_tcustom) {
            num_threads = SDL_GetCPUCount();
        }
        Thread_Initialize(num_threads);

        //Network_Initialize();

        if (game_ptr->IsClient()) {
            Window_Show();
            soundsystem = RG_NEW_CLASS(std_allocator, SoundSystem)();

            RG_NFDInit();
        }

        game_ptr->Initialize();
    }

    static String profiles[] = {
        "input",
        "worldupdate",
        "game",
        "physics",
        "audio",
        "systemupdate",
        "threads",
        "render",
        "other"
    };

    String GetProfile(Uint32 idx) {
        if (idx < 9) { return profiles[idx]; }
        return "null";
    }

    void Start() {
        rgLogInfo(RG_LOG_SYSTEM, "Starting engine...");
        PushEvent(0, RG_EVENT_STANDBY, NULL, NULL);

        timer.Update();

        double last_time = timer.GetTime();
        Uint32 frame = 0;

        while (running) {
            core_profiler->Reset();
            frame_time = timer.GetElapsedTime();
            timer.Update();
            uptime += frame_time;

            core_profiler->StartSection(profiles[0]);
            HandleEvents();
            UpdateInput();

            core_profiler->StartSection(profiles[1]);
            world->Update();

            core_profiler->StartSection(profiles[2]);
            game_ptr->MainUpdate();

            core_profiler->StartSection(profiles[3]);
            rgphysics->StepSimulation();

            if (game_ptr->IsClient()) {
                core_profiler->StartSection(profiles[4]);
                soundsystem->Update(GetDeltaTime());
                core_profiler->StartSection(profiles[5]);
                Render::UpdateSystems();
            }
            core_profiler->StartSection(profiles[6]);


            Thread_Execute();
            Thread_WaitForAll();

            if (game_ptr->IsClient()) {
                core_profiler->StartSection(profiles[7]);
                Render::Update();
            }

            core_profiler->StartSection(profiles[8]);
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

        if (game_ptr->IsClient()) {
            RG_NFDDestroy();
            RG_DELETE_CLASS(std_allocator, SoundSystem, soundsystem);
            Window_Destroy();
        }


        rgLogInfo(RG_LOG_SYSTEM, "Unloading game module...");
        PFN_MODULE_DESTROY ModuleDestroy = (PFN_MODULE_DESTROY)DL_GetProcAddress(game_module, "Module_Destroy");
        ModuleDestroy();
        DL_UnloadLibrary(game_module);

        Thread_Destroy();

        RG_DELETE_CLASS(GetDefaultAllocator(), World, world);
        RG_DELETE_CLASS(GetDefaultAllocator(), RGPhysics, rgphysics);

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
        rgLogWarn(RG_LOG_SYSTEM, "Engine: FORCE QUIT!");
        Logger_Destroy();
#if defined(RG_PLATFORM_WINDOWS)
        ExitProcess(0xFFFFFFFF);
#else
        exit(-1);
#endif
    }

    void HandleError(String message) {
        rgLogError(RG_LOG_SYSTEM, " * * * * * * * * * ENGINE ERROR * * * * * * * * *");
        rgLogError(RG_LOG_SYSTEM, "Engine: %s", message);
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

    SoundSystem* GetSoundSystem() {
        return soundsystem;
    }

    Profiler* GetProfiler() {
        return core_profiler;
    }

    RGPhysics* GetPhysics() {
        return rgphysics;
    }

}