#include "LM.h"
#include "TimeManager.h"

namespace Lumos
{

	TimeManager::TimeManager()
	{
		timer = new Timer();
		m_milliSeconds = 0.0f;
		m_seconds = 0.0f;
		m_minutes = 0;
		m_hours = 0;
		m_days = 0;
		m_timeUpdateSpeed = 100.0f;
		m_startSeconds = 0.0f;
	}

	TimeManager::~TimeManager()
	{
		delete timer;
	}

	float TimeManager::GetTimePercentage(float currentTime, float minSec, float maxSec)
	{
		float percentage = (maxSec - currentTime) / (maxSec - minSec);

		return	percentage;
	}

	void TimeManager::UpdateTime(bool Paused)
	{
		if (!Paused)
		{
			m_milliSeconds += timer->GetTimedMS() * m_timeUpdateSpeed;
			m_seconds = m_milliSeconds / 1000.0f;
			m_hours = static_cast<int>(floor(m_seconds / 3600.0f));
			m_minutes = (static_cast<int>(m_seconds) % 3600) / 60;
			if (m_seconds >= 86400)
			{
				m_milliSeconds = 0.0f;
				m_days++;
			}
		}
		else
		{
			timer->GetTimedMS();
		}
	}
}