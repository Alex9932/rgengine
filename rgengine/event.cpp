#define DLL_EXPORT
#include "event.h"

#include <vector>

namespace Engine {

    static std::vector<EventHandler> handlers;

    static SDL_Event event;
    static Uint32    rg_sdluserevent;

    static void UpdateHandlers(SDL_Event* event) {
        for (Uint32 i = 0; i < handlers.size(); ++i) {
            handlers[i](event);
        }
    }

    void Event_Initialize() {
        // Register event
        rg_sdluserevent = SDL_RegisterEvents(1);
        PushEvent(0xFFFFFFFF, RG_EVENT_START, NULL, NULL);
    }

    void Event_Destroy() {
        PushEvent(0xFFFFFFFF, RG_EVENT_STOP, NULL, NULL);
        handlers.clear();
    }

    void HandleEvents() {
        SDL_Event* event_ptr = &event;
        while (SDL_PollEvent(event_ptr)) {
            UpdateHandlers(event_ptr);
        }
    }

    void RegisterEventHandler(EventHandler handler) {
        handlers.push_back(handler);
        PushEvent(0xFFFFFFFF, RG_EVENT_NEW_HANDLER, (void*)handler, NULL); // Handler function's pointer in first argument
    }

    void FreeEventHandler(EventHandler handler) {
        std::vector<EventHandler>::iterator it;
        for (it = handlers.begin(); it != handlers.end(); it++) {
            if (*it == handler) {
                handlers.erase(it);
                break;
            }
        }

    }

    void PushEvent(Uint32 wnd_id, Sint32 code, void* ptr1, void* ptr2) {
        SDL_Event q_event;
        q_event.type = rg_sdluserevent;
        q_event.user.timestamp = SDL_GetTicks();
        q_event.user.windowID = wnd_id;
        q_event.user.code = code;
        q_event.user.data1 = ptr1;
        q_event.user.data2 = ptr2;
        //UpdateHandlers(&q_event);
        SDL_PushEvent(&q_event);
    }

    Uint32 GetUserEventID() {
        return rg_sdluserevent;
    }

}