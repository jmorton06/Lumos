#pragma once

#include "LM.h"
#include "Maths/Maths.h"
#include "Utilities/Timer.h"

namespace lumos
{

	class LUMOS_EXPORT TimeManager
	{
	public:
		TimeManager();
		~TimeManager();
        
        TimeManager(TimeManager const&) = delete;
        TimeManager& operator=(TimeManager const&) = delete;

		static float GetTimePercentage(float currentTime, float minSec, float maxSec);

		void UpdateTime(bool Paused);

		inline int GetMinutes() const
		{
			return m_minutes;
		}

		inline int GetHours() const
		{
			return m_hours;
		}

		inline int GetDays() const
		{
			return m_days;
		}

		inline float GetSeconds() const
		{
			return m_seconds;
		}

		inline float GetMilliSeconds() const
		{
			return m_milliSeconds;
		}

		inline void SetStartTime(int hour, int min)
		{
			m_milliSeconds = static_cast<float>(min * 60 + hour * 3600) * 1000.0f;
		}

		inline void SetTimeIncrease(float increase)
		{
			m_timeUpdateSpeed = increase;
		}

		inline float GetTimeIncrease() const
		{
			return m_timeUpdateSpeed;
		}

		Timer* GetTimer() const { return timer; }

	private:
		Timer* timer;
		float m_milliSeconds;
		float m_seconds;
		int m_minutes;
		int m_hours;
		int m_days;
		float m_timeUpdateSpeed = 100.0f;
		float m_startSeconds = 0.0f;
	};
}
