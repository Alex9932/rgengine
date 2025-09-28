/*
 * rgEngine logger.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: alex9932
 */

#define _CRT_SECURE_NO_WARNINGS

#define DLL_EXPORT
#include "logger.h"
#include "rgstring.h"
#include "engine.h"

#include <ctime>
#include <iomanip>

#ifdef RG_PLATFORM_LINUX
#include <sys/stat.h>
#endif

#define LOGGER_BUFFER_LENGTH 2048
#define LOGGER_LINES 20

#define LOGS_DIR "logs"

// TODO: Use FSWriter
static FILE* file_stream = NULL;

static char BUFFER[LOGGER_BUFFER_LENGTH];
static bool LOGGING_ENABLED = true;
static Uint32 MESSAGES = 0;

static char LOG_PATH[128];
static char LINES_BUFFER[LOGGER_LINES][LOGGER_BUFFER_LENGTH];
static SDL_LogPriority PRIORITY_BUFFER[LOGGER_LINES];
static Sint32 buffer_index = LOGGER_LINES - 1;

static SDL_Mutex* mutex;

#ifdef RG_PLATFORM_LINUX
RG_FORCE_INLINE static String getColor(SDL_LogPriority priority) {
    switch (priority) {
        case SDL_LOG_PRIORITY_VERBOSE:  return "1;34";
        case SDL_LOG_PRIORITY_DEBUG:    return "1;30";
        case SDL_LOG_PRIORITY_INFO:     return "1;32";
        case SDL_LOG_PRIORITY_WARN:     return "1;33";
        case SDL_LOG_PRIORITY_ERROR:    return "1;31";
        case SDL_LOG_PRIORITY_CRITICAL: return "0;41";
        default:                        return "1;34";
    }
}
#endif

#ifdef RG_PLATFORM_WINDOWS

static HANDLE console_handle;

RG_FORCE_INLINE static Uint8 getColor(SDL_LogPriority priority) {
    switch (priority) {
        case SDL_LOG_PRIORITY_VERBOSE:  return 1;
        case SDL_LOG_PRIORITY_DEBUG:    return 8;
        case SDL_LOG_PRIORITY_INFO:     return 2;
        case SDL_LOG_PRIORITY_WARN:     return 6;
        case SDL_LOG_PRIORITY_ERROR:    return 4;
        case SDL_LOG_PRIORITY_CRITICAL: return 64;
        default:                        return 1;
    }
}
#endif

RG_FORCE_INLINE static void printLine(SDL_LogPriority priority, String cat, String line) {
    String p = Engine::GetPriorityStr(priority);

    MESSAGES++;

    if (buffer_index < 0) {
        buffer_index = 0;
        for (Uint32 i = LOGGER_LINES - 1; i > 0; i--) {
            SDL_memcpy(LINES_BUFFER[i], LINES_BUFFER[i - 1], LOGGER_BUFFER_LENGTH);
            PRIORITY_BUFFER[i] = PRIORITY_BUFFER[i - 1];
        }
    }

    SDL_snprintf(LINES_BUFFER[buffer_index], LOGGER_BUFFER_LENGTH, "%s [%s] %s", p, cat, line);
    PRIORITY_BUFFER[buffer_index] = priority;

#if defined(RG_PLATFORM_WINDOWS)
    SetConsoleTextAttribute(console_handle, getColor(priority));
    OutputDebugStringA(LINES_BUFFER[buffer_index]);
    Uint64 length = SDL_strlen(LINES_BUFFER[buffer_index]);
    LPDWORD number_written = 0;
    WriteConsoleA(console_handle, LINES_BUFFER[buffer_index], (DWORD)length, number_written, 0);
    WriteConsoleA(console_handle, L"\n", 1, number_written, 0);
#elif defined(RG_PLATFORM_LINUX)
    printf("\033[%sm%s[%s] %s\033[0m\n", getColor(priority), p, cat, line);
#else
    printf("%s [%s]%s\n", p, cat, line);
#endif

    buffer_index--;

    if (file_stream) {
        fprintf(file_stream, "%s [%s] %s\n", p, cat, line);

        // Force write logs
        if (priority == SDL_LOG_PRIORITY_WARN ||
            priority == SDL_LOG_PRIORITY_ERROR ||
            priority == SDL_LOG_PRIORITY_CRITICAL/* ||
            priority == SDL_LOG_PRIORITY_INFO*/) {
            fflush(file_stream);
        }
    }

}

static Uint32 findEnd(Uint32 start, String str) {
    Uint32 len = (Uint32)SDL_strlen(str);
    for (Uint32 i = start; i < len; ++i) {
        if (str[i] == '\n') {
            return i;
        }
    }
    return len;
}

static void logger(void* userdata, int category, SDL_LogPriority priority, String message) {
    if (!LOGGING_ENABLED) {
        return;
    }

    if (category == RG_LOG_DEBUG && !Engine::IsDebug()) {
        return;
    }

    String cat = Engine::GetCategoryStr(category);

    Uint32 start = 0;
    Uint32 end   = 0;
    Uint32 len   = (Uint32)SDL_strlen(message);

    ///////////////////////////////
    // Lock mutex
    SDL_LockMutex(mutex);

    do {
        start = end;
        if (start != 0) start++;
        end = findEnd(start, message);
        SDL_memset(BUFFER, 0, LOGGER_BUFFER_LENGTH);
        SDL_memcpy(BUFFER, &message[start], end - start);
        printLine(priority, cat, BUFFER);
        //			printf("%d %d %d\n", start, end, len);
    } while (end != len);

    ////////////////////////////
    // Unlock mutex
    SDL_UnlockMutex(mutex);
}

namespace Engine {

    void Logger_Initialize() {
#if defined(RG_PLATFORM_WINDOWS)
        CreateDirectoryA(LOGS_DIR, NULL);
        console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
#else
        mkdir(LOGS_DIR, 0777);
#endif
        time_t tt = std::time(NULL);
        tm t = *std::localtime(&tt);
        SDL_snprintf(LOG_PATH, 128, "logs/%d-%d-%d %d%d%d.log", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        file_stream = fopen(LOG_PATH, "w");

        SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
        SDL_SetLogOutputFunction(logger, NULL);

        mutex = SDL_CreateMutex();
    }

    void Logger_Destroy() {
        SDL_DestroyMutex(mutex);
        fclose(file_stream);
    }

    String Logger_GetLine(Uint32 i) {
        return LINES_BUFFER[i];
    }

    SDL_LogPriority Logger_GetLinePriority(Uint32 i) {
        return PRIORITY_BUFFER[i];
    }

    Uint32 Logger_GetLines() {
        return LOGGER_LINES;
    }

    Uint32 Logger_GetMessages() {
        return MESSAGES;
    }

    String Logger_GetLogPath() {
        return LOG_PATH;
    }

}