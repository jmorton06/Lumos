#pragma once
#include "Core/Core.h"
#include "Core/Function.h"

namespace Lumos
{
    // "Something long is happening" state plus the panel that shows it. The
    // panel is built on the engine UI, so games get it as well as the editor.
    namespace Busy
    {
        // Scopes nest; the overlay shows while any is open.
        void Begin(const char* label);
        void End();
        bool IsBusy();

        void SetLabel(const char* label);
        // Line under the label ("Loading textures 12 / 40").
        void SetStep(const char* step);
        // 0..1, or negative when there is no way to tell.
        void SetProgress(float progress);

        const char* GetLabel();
        const char* GetStep();
        float GetProgress();

        // Draws one frame with only the overlay on it. For long work on the main
        // thread: call between steps and the window keeps painting. Does nothing
        // from inside a normal frame, off the main thread, or more than ~30 times a second.
        void Pump();

        // Runs `work` at the start of the next frame (outside rendering) with the
        // overlay up. The work can call Pump / SetStep / SetProgress as it goes.
        void Run(const char* label, const Function<void()>& work);

        // Builds the overlay into the current UI. opaque = nothing else is on screen.
        void Draw(bool opaque);
    }
}
