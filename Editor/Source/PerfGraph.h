#pragma once

namespace Lumos
{
    // Shared FPS/frametime history. Record() once per frame (status bar always runs),
    // tooltip helpers render the rolling graph on hover from anywhere.
    namespace PerfGraph
    {
        void Record();
        void FramerateTooltip();
        void FrametimeTooltip();
    }
}
