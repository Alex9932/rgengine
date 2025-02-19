/*
 * rgEngine core/input.h
 *
 *  Created on: Feb 11, 2022
 *      Author: alex9932
 */
#ifndef _INPUT_H
#define _INPUT_H

#include "rgtypes.h"

namespace Engine {

    namespace _Input {
        RG_DECLSPEC extern Float64 m_x;
        RG_DECLSPEC extern Float64 m_y;
        RG_DECLSPEC extern Float64 m_dx;
        RG_DECLSPEC extern Float64 m_dy;
        RG_DECLSPEC extern Float64 m_dw;
        RG_DECLSPEC extern Float64 m_sens;
    }

    void Input_Initialize();
    void Input_Destroy();
    void UpdateInput();

    void Input_StartConsole();

    RG_DECLSPEC Bool IsKeyDown(SDL_Scancode scancode);
    RG_DECLSPEC Bool IsButtonDown(Uint8 btn);

    RG_INLINE Float64 GetMouseX() { return _Input::m_x; }
    RG_INLINE Float64 GetMouseY() { return _Input::m_y; }
    RG_INLINE Float64 GetMouseDX() { return _Input::m_dx; }
    RG_INLINE Float64 GetMouseDY() { return _Input::m_dy; }
    RG_INLINE Float64 GetMouseDW() { return _Input::m_dw; }
    RG_INLINE Float64 GetMouseSensitivity() { return _Input::m_sens; }
    RG_INLINE void SetMouseSensitivity(Float64 sens) { _Input::m_sens = sens; }

}

#endif