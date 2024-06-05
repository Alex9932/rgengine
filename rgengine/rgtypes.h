/*
 * rgEngine core/rgtypes.h
 *
 *  Created on: Aug 23, 2022
 *      Author: alex9932
 */

#ifndef _RGTYPES_H
#define _RGTYPES_H

#include <SDL2/SDL.h>

#define RG_INLINE inline

#define _DISABLE_STRING_ANNOTATION

#if _WIN32 || _WIN64
	#define RG_FORCE_INLINE inline
	#define RG_PLATFORM_NAME "WIN32"
	#define RG_PLATFORM_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#ifdef DLL_EXPORT
	#define RG_DECLSPEC __declspec(dllexport)
	#else
	#define RG_DECLSPEC __declspec(dllimport)
	#endif
	typedef HMODULE LibraryHandle;
	typedef FARPROC ProcHandle;
	#if _WIN64
	#define RG_ENV64
	#else
	#define RG_ENV32
	#endif
#elif __GNUC__
	#define RG_FORCE_INLINE inline __attribute__((always_inline))
	#define RG_PLATFORM_NAME "LINUX"
	#define RG_PLATFORM_LINUX
	#define RG_DECLSPEC __attribute__((visibility("default")))
	typedef void* LibraryHandle;
	typedef void* ProcHandle;
	#if __x86_64__ || __ppc64__
	#define RG_ENV64
	#else
	#define RG_ENV32
	#endif
#else
	#define RG_FORCE_INLINE inline
	#define RG_PLATFORM_NAME "UNKNOWN"
	#define RG_DECLSPEC
	#define RG_ENV32
	#error "INVALID ENVIRONMENT"
#endif

#define RG_SET_FLAG(var, flag) (var |= flag)
#define RG_RESET_FLAG(var, flag) (var &= (~flag))
#define RG_CHECK_FLAG(var, flag) (var & flag)

#ifdef RG_ENV64
#define RG_CASTADDR(a) ((void*)(Uint64)(a))
#define RG_VPTR Uint64
#else
#define RG_CASTADDR(a) ((void*)(a))
#define RG_VPTR Uint32
#endif

#define RG_LOG_GAME   SDL_LOG_CATEGORY_APPLICATION
#define RG_LOG_SYSTEM SDL_LOG_CATEGORY_SYSTEM
#define RG_LOG_RENDER SDL_LOG_CATEGORY_RENDER
#define RG_LOG_ERROR  SDL_LOG_CATEGORY_ERROR
#define RG_LOG_ASSERT SDL_LOG_CATEGORY_ASSERT
#define RG_LOG_AUDIO  SDL_LOG_CATEGORY_AUDIO
#define RG_LOG_DEBUG  SDL_LOG_CATEGORY_CUSTOM

#define rgLogInfo     SDL_LogInfo
#define rgLogWarn     SDL_LogWarn
#define rgLogError    SDL_LogError
#define rgLogCritical SDL_LogCritical
#define rgLogDebug    SDL_LogDebug

typedef Uint16 Float16;
typedef float  Float32;
typedef double Float64;

typedef const char* String;
typedef const wchar_t* WString;

typedef bool Bool;

#define RG_DPI      3.1415926535   // Double PI
#define RG_HALF_DPI 1.57079632675
#define RG_PI       3.1415926535f  // Float PI
#define RG_HALF_PI  1.57079632675f
#define RG_EPSILONF FLT_EPSILON

#define RG_SIMD 1

#endif