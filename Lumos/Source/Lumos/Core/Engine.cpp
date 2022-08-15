#include "Precompiled.h"
#include "Engine.h"

namespace Lumos
{
    Engine::Engine()
        : m_MaxFramesPerSecond(1000.0 / 60.0)
    {
        m_TimeStep = new TimeStep();
    }

    Engine::~Engine()
    {
        delete m_TimeStep;
    }
}
