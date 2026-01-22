#include "Precompiled.h"
#include "Engine.h"

namespace Lumos
{
    Engine::Engine()
        : m_MaxFramesPerSecond(1000.0f / 60.0f)
    {
        m_TimeStep = new TimeStep();
        m_TimeStep->SetTargetFrameTime(m_MaxFramesPerSecond);
    }

    Engine::~Engine()
    {
        delete m_TimeStep;
    }
}
