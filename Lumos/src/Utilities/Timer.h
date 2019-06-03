#pragma once
#include "LM.h"

#ifdef LUMOS_PLATFORM_WINDOWS
#include <windows.h>
typedef LARGE_INTEGER TimeStamp;
#else
#include <chrono>
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimeStamp;
#endif

namespace Lumos
{

	class LUMOS_EXPORT Timer
	{
	public:

		Timer() : m_Start(Now()) { m_LastTime = m_Start; }
		~Timer() = default;

		float	GetTimedMS();

		static TimeStamp Now();

		static double Duration(TimeStamp start, TimeStamp end, double timeResolution = 1.0);
		static float  Duration(TimeStamp start, TimeStamp end, float timeResolution);

		float GetMS(const float timeResolution) const
		{
			return Duration(m_Start, Now(), timeResolution);
		}
		double GetMS(const double timeResolution = 1.0) const
		{
			return Duration(m_Start, Now(), timeResolution);
		}

	protected:
		TimeStamp m_Start;		//Start of timer
		TimeStamp m_Frequency;	//Ticks Per Second
		TimeStamp m_LastTime;	//Last time GetTimedMS was called
	};

}
