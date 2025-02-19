#define DLL_EXPORT
#include "input.h"
#include "event.h"

#include "engine.h"

#include <iostream>
#include <string>
#include <thread>
#include <mutex>

#include "command.h"

#define RG_MAX_KEYS 1024
#define RG_MAX_BTNS 64

namespace Engine {

    namespace _Input {
        Float64 m_x = 0.0;
        Float64 m_y = 0.0;
        Float64 m_dx = 0.0;
        Float64 m_dy = 0.0;
        Float64 m_dw = 0.0;
        Float64 m_sens = 0.07;
    }
    static Float64 m_tdx = 0.0;
    static Float64 m_tdy = 0.0;
    static Float64 m_tdw = 0.0;
    static Bool   m_keys[RG_MAX_KEYS];
    static Bool   m_btns[RG_MAX_BTNS];

    static Bool console_started = false;

    // TODO: Test on linux

#ifdef RG_PLATFORM_WINDOWS
    static HANDLE consoleStopEvent;
#else
    static int consoleStopPipe[2];
#endif

    static std::mutex              console_mtx;
    static std::condition_variable console_condvar;
    static std::thread             console_thr;

    static bool InputHandler(SDL_Event* event) {
        if (event->type == SDL_MOUSEMOTION) {
            m_tdx += ((double)event->motion.x - _Input::m_x) * _Input::m_sens;
            m_tdy += ((double)event->motion.y - _Input::m_y) * _Input::m_sens;
            _Input::m_x = (double)event->motion.x;
            _Input::m_y = (double)event->motion.y;
        }

        if (event->type == SDL_MOUSEWHEEL) {
            m_tdw += event->wheel.y;
        }

        if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
            if (event->key.keysym.scancode >= RG_MAX_KEYS) { return true; }
            m_keys[event->key.keysym.scancode] = (event->type == SDL_KEYUP ? false : true);
        }

        if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
            if (event->button.button >= RG_MAX_BTNS) { return true; }
            m_btns[event->button.button] = (event->type == SDL_MOUSEBUTTONUP ? false : true);
        }

        if (event->type == GetUserEventID() && event->user.code == RG_EVENT_STANDBY) {
            console_condvar.notify_one();
        }

        return true;
    }

    void Input_Initialize() {
        RegisterEventHandler(InputHandler);
        SDL_memset(m_keys, 0, sizeof(m_keys));
        SDL_memset(m_btns, 0, sizeof(m_btns));
    }

    void Input_Destroy() {
        FreeEventHandler(InputHandler);
        if (console_started) {
            console_started = false;

#ifdef RG_PLATFORM_WINDOWS
            SetEvent(consoleStopEvent);
#else
            write(consoleStopPipe[1], "x", 1);
#endif

            console_thr.join(); // Wait command-line thread

#ifdef RG_PLATFORM_WINDOWS
            CloseHandle(consoleStopEvent);
#else
            close(consoleStopPipe[0]);
            close(consoleStopPipe[1]);
#endif
        }
    }

    void UpdateInput() {
        _Input::m_dx = m_tdx;
        _Input::m_dy = m_tdy;
        _Input::m_dw = m_tdw;
        m_tdx = 0;
        m_tdy = 0;
        m_tdw = 0;
    }

    bool IsKeyDown(SDL_Scancode scancode) {
        return m_keys[scancode];
    }

    bool IsButtonDown(Uint8 btn) {
        return m_btns[btn];
    }

    //////////////// CONSOLE ////////////////


    static Uint32 IsBufferAvailable() {
        Uint32 status = 0;

#ifdef RG_PLATFORM_WINDOWS
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE handles[2] = { hStdin, consoleStopEvent };
        DWORD result = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        if (result == WAIT_OBJECT_0)     { status = 1; }
        if (result == WAIT_OBJECT_0 + 1) { status = 2; }

#else

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(stopPipe[0], &fds);

        int max_fd = std::max(STDIN_FILENO, stopPipe[0]) + 1;
        int result = select(max_fd, &fds, nullptr, nullptr, nullptr);

        if (result > 0) {
            if (FD_ISSET(STDIN_FILENO, &fds)) { status = 1; }
            if (FD_ISSET(stopPipe[0], &fds))  { status = 2; }
        }
#endif

        return status;
    }

    static void ReadBuffer(char* str, Uint32 len) {
        std::string input;
        std::getline(std::cin, input);
        SDL_strlcpy(str, input.c_str(), len);
    }

    static RG_INLINE void PrintPromt() {
        printf("rgengine> ");
        fflush(stdout);
    }

    static void ConsoleProc() {
        rgLogInfo(RG_LOG_SYSTEM, "Starting console thread");

        // Wait cond var
        std::unique_lock<std::mutex> lock(console_mtx);
        console_condvar.wait(lock);

        char cmdbuffer[256];

        PrintPromt();
        while (console_started) {
            Uint32 status = IsBufferAvailable();
            if (status == 1) {
                ReadBuffer(cmdbuffer, 256);
                ExecuteCommand(cmdbuffer);
                PrintPromt();
            } else if (status == 2) {
                break;
            }
        }

        rgLogInfo(RG_LOG_SYSTEM, "Stopped console thread");
    }

    void Input_StartConsole() {
        if (console_started) { return; }
        console_started = true;

#ifdef RG_PLATFORM_WINDOWS
        consoleStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
        pipe(consoleStopPipe);
        fcntl(consoleStopPipe[0], F_SETFL, O_NONBLOCK); // to thread function
#endif

        console_thr = std::thread(ConsoleProc);
    }

}