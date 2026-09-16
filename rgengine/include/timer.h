/*
 * rgEngine timer.h
 *
 *  Created on: Feb 13, 2022
 *      Author: alex9932
 */
#ifndef _TIMER_H
#define _TIMER_H

#include "rgtypes.h"

namespace Engine {

    class Timer {
        private:
            Uint64  m_time;
            Float64 time;

        public:
            RG_DECLSPEC Timer();
            RG_INLINE   virtual ~Timer() {}

            RG_DECLSPEC Float64 GetElapsedTime(); // In seconds
            RG_INLINE   Float64 GetTime() { return time; } // In seconds
            RG_DECLSPEC void Update();
    };

}

#endif