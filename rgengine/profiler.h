#ifndef _PROFILER_H
#define _PROFILER_H

#include "rgtypes.h"
#include <map>

namespace Engine {

    class Profiler {
        private:
            std::map<Uint32, Float64> m_sections;
            Uint32                    m_current;
            Uint64                    m_time;

        public:
            Profiler();
            virtual ~Profiler();

            void Reset();
            void StartSection(String name);
            Float64 GetTime(String section);
            Float64 GetTotalTime();

    };

}

#endif