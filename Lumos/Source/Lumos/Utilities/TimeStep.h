#pragma once

namespace Lumos
{
    class LUMOS_EXPORT TimeStep
    {
    public:
        explicit inline TimeStep(float initialTime)
            : m_Timestep(0.0f)
            , m_LastTime(initialTime)
            , m_Elapsed(0.0f)
        {
        }

        inline void Update(float currentTime)
        {
            m_Timestep = currentTime - m_LastTime;
            m_LastTime = currentTime;
            m_Elapsed += m_Timestep;
        }

        inline float GetMillis() const { return m_Timestep * 1000.0f; }
        inline float GetElapsedMillis() const { return m_Elapsed * 1000.0f; }

        inline float GetSeconds() const { return m_Timestep; }
        inline float GetElapsedSeconds() const { return m_Elapsed; }

    private:
        float m_Timestep; // Seconds
        float m_LastTime;
        float m_Elapsed;
    };

}
