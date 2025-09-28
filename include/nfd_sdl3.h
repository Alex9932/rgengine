/*
  Native File Dialog Extended
  Repository: https://github.com/btzy/nativefiledialog-extended
  License: Zlib
  Authors: Bernard Teo, Alex9932

  This header contains a function to convert an SDL window handle to a native window handle for
  passing to NFDe.

  This is meant to be used with SDL2, but if there are incompatibilities with future SDL versions,
  we can conditionally compile based on SDL_MAJOR_VERSION.

  Modification of nfd_sdl2.h for SDL3.
 */

#ifndef _NFD_SDL2_H
#define _NFD_SDL2_H

#include <SDL3/SDL_error.h>
#include <nfd.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#define NFD_INLINE inline
#else
#define NFD_INLINE static inline
#endif  // __cplusplus

/**
 *  Converts an SDL window handle to a native window handle that can be passed to NFDe.
 *  @param sdlWindow The SDL window handle.
 *  @param[out] nativeWindow The output native window handle, populated if and only if this function
 *  returns true.
 *  @return Either true to indicate success, or false to indicate failure.  If false is returned,
 * you can call SDL_GetError() for more information.  However, it is intended that users ignore the
 * error and simply pass a value-initialized nfdwindowhandle_t to NFDe if this function fails. */
NFD_INLINE bool NFD_GetNativeWindowFromSDLWindow(SDL_Window* sdlWindow,
                                                 nfdwindowhandle_t* nativeWindow) {

    SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);

    // SDL3 migration
#if defined(SDL_PLATFORM_WIN32)
    nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
    nativeWindow->handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    return true;
#elif defined(SDL_PLATFORM_COCOA)
    nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_COCOA;
    nativeWindow->handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    return true;
#elif defined(SDL_PLATFORM_X11)
    nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_X11;
    nativeWindow->handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, NULL);
    return true;
#else
    (void)nativeWindow;
    SDL_SetError("Unsupported native window type.");
    return false;
#endif

}

#undef NFD_INLINE
#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _NFD_SDL2_H
