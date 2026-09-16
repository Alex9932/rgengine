/*
 * rgEngine logger.h
 *
 *  Created on: Feb 10, 2022
 *      Author: alex9932
 */

#ifndef _LOGGER_H
#define _LOGGER_H

#include "rgtypes.h"

namespace Engine {

    void Logger_Initialize();
    void Logger_Destroy();

    RG_DECLSPEC SDL_LogPriority Logger_GetLinePriority(Uint32 i);
    RG_DECLSPEC String Logger_GetLine(Uint32 i);
    RG_DECLSPEC Uint32 Logger_GetLines();
    RG_DECLSPEC Uint32 Logger_GetMessages();
    RG_DECLSPEC String Logger_GetLogPath();

}

#endif