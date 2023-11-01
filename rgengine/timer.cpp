#define DLL_EXPORT
#include "timer.h"

namespace Engine {

	Timer::Timer() {
		this->m_time = SDL_GetPerformanceCounter();
		this->time = 0;
	}

	Timer::~Timer() {}

	double Timer::GetElapsedTime() {
		return (double)(SDL_GetPerformanceCounter() - this->m_time) / (double)SDL_GetPerformanceFrequency();
	}

	double Timer::GetTime() {
		return time;
	}

	void Timer::Update() {
		double delta = (double)(SDL_GetPerformanceCounter() - this->m_time) / (double)SDL_GetPerformanceFrequency();
		m_time = SDL_GetPerformanceCounter();
		time += delta;
	}

}