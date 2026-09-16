#ifndef _PROFILER_H
#define _PROFILER_H

#include "rgtypes.h"
#include <map>

namespace Engine {

#define _RG_PROFILER_MAP std::map<Uint32, Float64>

    class Profiler {
        private:
            _RG_PROFILER_MAP m_sections;
            Uint32           m_current;
            Uint64           m_time;

        public:
            RG_DECLSPEC Profiler();
            RG_DECLSPEC virtual ~Profiler();

            RG_DECLSPEC void Reset();
            RG_DECLSPEC void StartSection(String name);
            RG_DECLSPEC Float64 GetTime(String section);
            RG_DECLSPEC Float64 GetTotalTime();

            RG_INLINE Uint32 GetSectionCount() { return (Uint32)m_sections.size(); }

    };

}

#endif