#pragma once

namespace Lumos
{
    class Timer;
    class LUMOS_EXPORT TimeStep
    {
    public:
        static constexpr int FRAME_HISTORY_SIZE = 11;

        TimeStep();
        ~TimeStep();

        void OnUpdate();

        inline double GetMillis() const { return m_SmoothedTimestep; }
        inline double GetElapsedMillis() const { return m_Elapsed; }

        inline double GetSeconds() const { return m_SmoothedTimestep * 0.001; }
        inline double GetElapsedSeconds() const { return m_Elapsed * 0.001; }

        inline double GetRawMillis() const { return m_Timestep; }
        inline double GetRawSeconds() const { return m_Timestep * 0.001; }

        void SetTargetFrameTime(double targetMs) { m_TargetFrameTime = targetMs; }
        double GetTargetFrameTime() const { return m_TargetFrameTime; }

        void SetFrameSmoothing(bool enabled) { m_SmoothingEnabled = enabled; }
        bool GetFrameSmoothing() const { return m_SmoothingEnabled; }

    private:
        double m_Timestep;
        double m_SmoothedTimestep;
        double m_LastTime;
        double m_Elapsed;
        double m_TargetFrameTime = 0.0;

        double m_FrameHistory[FRAME_HISTORY_SIZE] = {};
        int m_FrameHistoryIndex                   = 0;
        bool m_FrameHistoryFilled                 = false;
        bool m_SmoothingEnabled                   = true;

        Timer* m_Timer = nullptr;

        double CalculateSmoothedDelta();
    };

}
