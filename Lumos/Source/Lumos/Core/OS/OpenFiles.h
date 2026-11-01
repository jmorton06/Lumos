#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // Files the OS asks the app to open: Finder "Open With" / double-click, a drop on the Dock icon, `open -a`.
    namespace OpenFiles
    {
        // Hooks the platform (macOS: the app delegate). Called once the window system is up.
        void Install();
        LUMOS_EXPORT void Push(const std::string& path); // also for tests
        LUMOS_EXPORT void Pop(TDArray<std::string>& out);
    }
}
