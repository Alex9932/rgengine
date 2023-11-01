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

    class RG_DECLSPEC Timer {
        private:
            Uint64  m_time;
            Float64 time;

        public:
            Timer();
            virtual ~Timer();

            Float64 GetElapsedTime(); // In seconds
            Float64 GetTime(); // In seconds
            void Update();
    };

}

#endif