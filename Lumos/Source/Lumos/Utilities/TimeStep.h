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

        // Gameplay delta - scaled by the time scale (0 while paused).
        inline double GetMillis() const { return m_SmoothedTimestep * m_TimeScale; }
        inline double GetElapsedMillis() const { return m_Elapsed; }

        inline double GetSeconds() const { return m_SmoothedTimestep * 0.001 * m_TimeScale; }
        inline double GetElapsedSeconds() const { return m_Elapsed * 0.001; }

        // Wall clock, never scaled: stats, profiling, and anything that must keep
        // running while the game is paused (ImGui and the debug overlay both do).
        inline double GetRawMillis() const { return m_Timestep; }
        inline double GetRawSeconds() const { return m_Timestep * 0.001; }
        inline double GetUnscaledMillis() const { return m_Timestep; }

        void SetTargetFrameTime(double targetMs) { m_TargetFrameTime = targetMs; }
        double GetTargetFrameTime() const { return m_TargetFrameTime; }

        void SetFrameSmoothing(bool enabled) { m_SmoothingEnabled = enabled; }
        bool GetFrameSmoothing() const { return m_SmoothingEnabled; }

        // >0 pins every frame to this many ms, decoupling updates from the wall clock.
        void SetFixedTimestep(double ms) { m_FixedTimestep = ms; }
        double GetFixedTimestep() const { return m_FixedTimestep; }

        // Scales the delta handed to gameplay. 0 = paused, 1 = normal.
        void SetTimeScale(double scale) { m_TimeScale = scale; }
        double GetTimeScale() const { return m_TimeScale; }

    private:
        double m_Timestep;
        double m_SmoothedTimestep;
        double m_LastTime;
        double m_Elapsed;
        double m_TargetFrameTime = 0.0;
        double m_FixedTimestep   = 0.0;
        double m_TimeScale       = 1.0;

        double m_FrameHistory[FRAME_HISTORY_SIZE] = {};
        int m_FrameHistoryIndex                   = 0;
        bool m_FrameHistoryFilled                 = false;
        bool m_SmoothingEnabled                   = true;

        Timer* m_Timer = nullptr;

        double CalculateSmoothedDelta();
    };

}
