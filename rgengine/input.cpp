#define DLL_EXPORT
#include "input.h"
#include "event.h"

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
    static Bool   keys[1024];
    static Bool   m_btns[64];

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
            keys[event->key.keysym.scancode] = (event->type == SDL_KEYUP ? false : true);
        }

        if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
            m_btns[event->button.button] = (event->type == SDL_MOUSEBUTTONUP ? false : true);
        }

        return true;
    }

    void Input_Initialize() {
        RegisterEventHandler(InputHandler);
        SDL_memset(keys, 0, sizeof(keys));
        SDL_memset(m_btns, 0, sizeof(m_btns));
    }

    void Input_Destroy() {

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
        return keys[scancode];
    }

    bool IsButtonDown(Uint8 btn) {
        return m_btns[btn];
    }

}