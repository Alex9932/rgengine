/*
 * rgEngine uuid.h
 *
 *  Created on: Jan 26, 2023
 *      Author: alex9932
 */
#ifndef _UUID_H
#define _UUID_H

#include "rgtypes.h"

#define UUID_DEFINED
typedef Uint64 UUID;

namespace Engine {

    RG_DECLSPEC UUID GenerateUUID();

}

#endif