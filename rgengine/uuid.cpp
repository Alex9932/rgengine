/*
 * rgEngine uuid.cpp
 *
 *  Created on: Jan 26, 2023
 *      Author: alex9932
 */

#define DLL_EXPORT

#include "uuid.h"
#include <random>

static std::random_device rDevice;
static std::mt19937_64 rEngine(rDevice());
static std::uniform_int_distribution<Uint64> rUniformDistr;

namespace Engine {
    RGUUID GenerateUUID() { return rUniformDistr(rEngine); }
}
