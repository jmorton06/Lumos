#pragma once

#include "LM.h"
#include "Utilities/TSingleton.h"

//#define LUMOS_LIMIT_FRAMERATE

namespace Lumos
{
    class Timer;

    class LUMOS_EXPORT Engine : public TSingleton<Engine>
    {
        friend class TSingleton<Engine>;

    public:
        Engine();
        ~Engine();

        uint  GetFPS() const { return m_FramesPerSecond;  }
        uint  GetUPS() const { return m_UpdatesPerSecond; }
        float GetFrametime() const { return m_Frametime;  }
        float TargetFrameRate() const { return m_MaxFramesPerSecond; }

        void SetFPS(uint fps) { m_FramesPerSecond = fps;  }
        void SetUPS(uint ups) { m_UpdatesPerSecond = ups; }
        void SetFrametime(float frameTime) { m_Frametime = frameTime;  }
        void SetTargetFrameRate(float targetFPS) { m_MaxFramesPerSecond = targetFPS; }

    private:

        uint  m_UpdatesPerSecond;
        uint  m_FramesPerSecond;
        float m_Frametime = 0.1f;
        float m_MaxFramesPerSecond;
    };
}
