
// cookiedough -- high perf. timer (SDL)

#pragma once

#include "../3rdparty/SDL2-2.0.8/include/SDL.h"

class Timer
{
public:
	Timer()
	{
		Reset();
	}

	void Reset()
	{
		m_frequency = SDL_GetPerformanceFrequency();
		m_oneOverFreq = 1.f/m_frequency;
		m_offset = SDL_GetPerformanceCounter();
	}

	float Get() const 
	{
		const uint64_t time = SDL_GetPerformanceCounter();
		return (time-m_offset)*m_oneOverFreq;
	}

private:
	uint64_t m_frequency; 
	uint64_t m_offset;
	float m_oneOverFreq;
};
