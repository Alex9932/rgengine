/*
 * rgEngine uuid.h
 *
 *  Created on: Jan 26, 2023
 *      Author: alex9932
 */
#ifndef _UUID_H
#define _UUID_H

#include "rgtypes.h"

typedef Uint64 RGUUID;

namespace Engine {

    RG_DECLSPEC RGUUID GenerateUUID();

}

#endif