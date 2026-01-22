#include "Precompiled.h"
#include "TimeStep.h"
#include "Timer.h"
#include <thread>
#include <algorithm>

namespace Lumos
{
    TimeStep::TimeStep()
        : m_Timestep(0.0)
        , m_SmoothedTimestep(0.0)
        , m_LastTime(0.0)
        , m_Elapsed(0.0)
        , m_TargetFrameTime(0.0)
    {
        m_Timer = new Timer();

        for(int i = 0; i < FRAME_HISTORY_SIZE; i++)
            m_FrameHistory[i] = 1 / 60;
    }

    TimeStep::~TimeStep()
    {
        delete m_Timer;
    }

    double TimeStep::CalculateSmoothedDelta()
    {
        double sorted[FRAME_HISTORY_SIZE];
        for(int i = 0; i < FRAME_HISTORY_SIZE; i++)
            sorted[i] = m_FrameHistory[i];

        std::sort(sorted, sorted + FRAME_HISTORY_SIZE);

        const int skipCount = 2;
        double sum          = 0.0;
        int count           = 0;

        for(int i = skipCount; i < FRAME_HISTORY_SIZE - skipCount; i++)
        {
            sum += sorted[i];
            count++;
        }

        return count > 0 ? sum / count : m_Timestep;
    }

    void TimeStep::OnUpdate()
    {
        double currentTime = m_Timer->GetElapsedMSD();
        double dt          = currentTime - m_LastTime;

        if(m_TargetFrameTime > 0.0)
        {
            LUMOS_PROFILE_SCOPE("Sleep TimeStep to target fps");
            double remainingTime = m_TargetFrameTime - dt;

            if(remainingTime > 1.0)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int64_t>((remainingTime - 1.0) * 1000.0)));
            }

            currentTime = m_Timer->GetElapsedMSD();
            dt          = currentTime - m_LastTime;
            while(dt < m_TargetFrameTime)
            {
                currentTime = m_Timer->GetElapsedMSD();
                dt          = currentTime - m_LastTime;
            }
        }

        currentTime = m_Timer->GetElapsedMSD();
        m_Timestep  = currentTime - m_LastTime;
        m_LastTime  = currentTime;

        m_Timestep = std::max(0.1, std::min(m_Timestep, 250.0));

        m_FrameHistory[m_FrameHistoryIndex] = m_Timestep;
        m_FrameHistoryIndex                 = (m_FrameHistoryIndex + 1) % FRAME_HISTORY_SIZE;
        if(m_FrameHistoryIndex == 0)
            m_FrameHistoryFilled = true;

        if(m_SmoothingEnabled && m_FrameHistoryFilled)
            m_SmoothedTimestep = CalculateSmoothedDelta();
        else
            m_SmoothedTimestep = m_Timestep;

        m_Elapsed += m_Timestep;
    }
}
