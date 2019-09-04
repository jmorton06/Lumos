#include "LM.h"
#include "Engine.h"

namespace Lumos
{
    Engine::Engine() : m_MaxFramesPerSecond(1000.0f / 60.0f), m_UpdatesPerSecond(0), m_FramesPerSecond(0),
                       m_Frametime(0)
    {
        m_TimeStep = lmnew TimeStep(0.0f);
    }

    Engine::~Engine()
    {
        lmdel m_TimeStep;
    }
}

