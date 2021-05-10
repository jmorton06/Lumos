#pragma once

#include "Timer.h"
#include "Maths/Vector4.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    class LUMOS_EXPORT PerformanceTimer
    {
    public:
        PerformanceTimer()
            : m_UpdateInterval(1.0f)
            , m_RealTimeElapsed(0.0f)
        {
            m_Timer.GetTimedMS();
            memset(&m_CurrentData, 0, sizeof(PerfTimer_Data));
            memset(&m_PreviousData, 0, sizeof(PerfTimer_Data));
        }

        virtual ~PerformanceTimer() { }

        inline float GetHigh() const { return m_PreviousData._max; }
        inline float GetLow() const { return m_PreviousData._min; }
        inline float GetAvg() const { return m_PreviousData._sum / float(m_PreviousData._num); }

        void SetUpdateInterval(float seconds) { m_UpdateInterval = seconds; }

        void BeginTimingSection()
        {
            m_Timer.GetTimedMS();
        }

        void EndTimingSection()
        {
            float elapsed = m_Timer.GetTimedMS();

            if(m_CurrentData._num == 0)
            {
                m_CurrentData._max = elapsed;
                m_CurrentData._min = elapsed;
            }
            else
            {
                m_CurrentData._max = Maths::Max(m_CurrentData._max, elapsed);
                m_CurrentData._min = Maths::Min(m_CurrentData._min, elapsed);
            }

            m_CurrentData._num++;
            m_CurrentData._sum += elapsed;
        }

        void UpdateRealElapsedTime(float dt)
        {
            m_RealTimeElapsed += dt;
            if(m_RealTimeElapsed >= m_UpdateInterval)
            {
                m_RealTimeElapsed -= m_UpdateInterval;
                m_PreviousData = m_CurrentData;
                memset(&m_CurrentData, 0, sizeof(PerfTimer_Data));
            }
        }

    protected:
        float m_UpdateInterval;
        float m_RealTimeElapsed;

        Timer m_Timer;

        struct PerfTimer_Data
        {
            float _max;
            float _min;

            float _sum;
            int _num;
        };

        PerfTimer_Data m_PreviousData;
        PerfTimer_Data m_CurrentData;
    };
}
