#pragma once

#include "Core/Core.h"
#include "Utilities/TimeStep.h"
#include "Utilities/TSingleton.h"

namespace Lumos
{
    class LUMOS_EXPORT Engine : public ThreadSafeSingleton<Engine>
    {
        friend class TSingleton<Engine>;

    public:
        Engine();
        ~Engine();

        float TargetFrameRate() const { return m_MaxFramesPerSecond; }
        void SetTargetFrameRate(float targetFPS) { m_MaxFramesPerSecond = targetFPS; }

        static TimeStep& GetTimeStep() { return Engine::Get().m_TimeStep; }

        struct Stats
        {
            uint32_t UpdatesPerSecond;
            uint32_t FramesPerSecond;
            uint32_t NumRenderedObjects = 0;
            uint32_t NumShadowObjects = 0;
            uint32_t NumDrawCalls = 0;
            float FrameTime = 0.0f;
            float UsedGPUMemory = 0.0f;
            float UsedRam = 0.0f;
            float TotalGPUMemory = 0.0f;
        };

        void ResetStats()
        {
            m_Stats.NumRenderedObjects = 0;
            m_Stats.NumShadowObjects = 0;
            m_Stats.FrameTime = 0.0f;
            m_Stats.UsedGPUMemory = 0.0f;
            m_Stats.UsedRam = 0.0f;
            m_Stats.NumDrawCalls = 0;
            m_Stats.TotalGPUMemory = 0.0f;
        }

        Stats& Statistics() { return m_Stats; }

    private:
        Stats m_Stats;
        float m_MaxFramesPerSecond;
        TimeStep m_TimeStep;
    };
}
