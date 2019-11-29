#pragma once
#include "lmpch.h"

namespace Lumos
{
    class LUMOS_EXPORT TimeStep
    {
    public:
        explicit _FORCE_INLINE_ TimeStep(float initialTime)
                : m_Timestep(0.0f), m_LastTime(initialTime), m_Elapsed(0.0f)
        {
        }

        _FORCE_INLINE_ void Update(float currentTime)
        {
            m_Timestep = currentTime - m_LastTime;
            m_LastTime = currentTime;
            m_Elapsed += m_Timestep;
        }

        _FORCE_INLINE_ float GetMillis() const { return m_Timestep; }
        _FORCE_INLINE_ float GetElapsedMillis() const { return m_Elapsed; }

        _FORCE_INLINE_ float GetSeconds() const { return m_Timestep * 0.001f; }
        _FORCE_INLINE_ float GetElapsedSeconds() const { return m_Elapsed * 0.001f; }

    private:
        float m_Timestep;
        float m_LastTime;
        float m_Elapsed;
    };

}
