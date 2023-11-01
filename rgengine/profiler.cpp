#define DLL_EXPORT
#include "profiler.h"
#include "rgmath.h"

namespace Engine {

    Profiler::Profiler() {
        this->Reset();
        // ???
        std::map<Uint32, Float64>::iterator it = this->m_sections.begin();
        while (it != this->m_sections.end()) { it->second = 0; it++; }
    }

    Profiler::~Profiler() { this->m_sections.clear(); }

    void Profiler::Reset() {
        this->m_current = 0;
    }

    void Profiler::StartSection(String name) {
        if (this->m_current) { // != 0
            Float64 elapsed = (Float64)(SDL_GetPerformanceCounter() - this->m_time) / (Float64)SDL_GetPerformanceFrequency();
            this->m_sections[this->m_current] = elapsed;
        }

        this->m_time = SDL_GetPerformanceCounter();
        this->m_current = rgCRC32(name, (Uint32)SDL_strlen(name));
    }

    Float64 Profiler::GetTime(String section) {
        Uint32 key = rgCRC32(section, (Uint32)SDL_strlen(section));
        return this->m_sections[key];
    }

    Float64 Profiler::GetTotalTime() {
        std::map<Uint32, Float64>::iterator it = this->m_sections.begin();
        Float64 total = 0;
        while (it != this->m_sections.end()) { total += it->second; it++; }
        return total;
    }

}