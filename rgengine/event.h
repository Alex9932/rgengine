/*
 * rgEngine core/event.h
 *
 *  Created on: Feb 11, 2022
 *      Author: alex9932
 */
#ifndef _EVENT_H
#define _EVENT_H

#include "rgtypes.h"

/*
    Engine events: 0x00000000 - 0x0000FFFF
    Render events: 0x00010000 - 0x0001FFFF

    Reserved: 0x00020000 - 0xFFFFFFFE

    Not a event id: 0xFFFFFFFF
 */

#define RG_EVENT_NOP           0xFFFFFFFF
#define RG_EVENT_STOP          0x00000001
#define RG_EVENT_START         0x00000002
#define RG_EVENT_NEW_HANDLER   0x00000003
#define RG_EVENT_COMMAND       0x00000004 // Is command entered
#define RG_EVENT_STANDBY       0x00000005 // Engine pushes this event after first initialization

#define RG_EVENT_ONLEVELSAVE   0x00000006
#define RG_EVENT_ONLEVELLOAD   0x00000007

#define RG_EVENT_ONCVARCHANGED 0x00000008 // ptr1 = changed cvar, ptr2 = NULL

#define RG_EVENT_SHUTDOWN_RQ   0x00000009

#define RG_EVENT_ONLOADINGSTARTED  0x0000000A
#define RG_EVENT_ONLOADINGFINISHED 0x0000000B

typedef bool (*EventHandler)(SDL_Event*);

namespace Engine {

    void Event_Initialize();
    void Event_Destroy();
    void HandleEvents();

    RG_DECLSPEC void RegisterEventHandler(EventHandler handler);
    RG_DECLSPEC void FreeEventHandler(EventHandler handler);
    RG_DECLSPEC void PushEvent(Uint32 wnd_id, Sint32 code, void* ptr1, void* ptr2);

    RG_DECLSPEC Uint32 GetUserEventID();

}

#endif