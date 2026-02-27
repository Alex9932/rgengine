#define DLL_EXPORT
#include "event.h"

#include <vector>

namespace Engine {
	struct HandlerPair {
        union {
            void*           rawptr;
		    EventHandler    pfn;
            EventHandlerArg pfn_arg;
        };
		void*        user_data;
	};

    static std::vector<HandlerPair> handlers;

    static SDL_Event event;
    static Uint32    rg_sdluserevent;

    static void UpdateHandlers(SDL_Event* event) {
        for (Uint32 i = 0; i < handlers.size(); ++i) {
            if (handlers[i].user_data) {
                handlers[i].pfn_arg(event, handlers[i].user_data);
            } else {
                handlers[i].pfn(event);
            }
        }
    }

    void Event_Initialize() {
        // Register event
        rg_sdluserevent = SDL_RegisterEvents(1);
        PushEvent(0xFFFFFFFF, RG_EVENT_START, NULL, NULL);
    }

    void Event_Destroy() {
        PushEvent(0xFFFFFFFF, RG_EVENT_STOP, NULL, NULL);
        HandleEvents();
        handlers.clear();
    }

    void HandleEvents() {
        SDL_Event* event_ptr = &event;
        while (SDL_PollEvent(event_ptr)) {
            UpdateHandlers(event_ptr);
        }
    }

    static void _RegisterEventHandler(void* handler, void* userdata) {
        HandlerPair pair = {};
        pair.rawptr    = handler;
        pair.user_data = userdata;
        handlers.push_back(pair);
        PushEvent(0xFFFFFFFF, RG_EVENT_NEW_HANDLER, (void*)handler, NULL); // Handler function's pointer in first argument
    }

    void RegisterEventHandler(EventHandler handler) {
		_RegisterEventHandler((void*)handler, NULL);
    }

    void RegisterEventHandler(EventHandlerArg handler, void* userdata) {
		_RegisterEventHandler((void*)handler, userdata);
    }

    void FreeEventHandler(void* handler) {
        std::vector<HandlerPair>::iterator it;
        for (it = handlers.begin(); it != handlers.end(); it++) {
            if ((*it).rawptr == handler) {
                //handlers.erase(it);

                *it = std::move(handlers.back());
                handlers.pop_back();
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