#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // Recursive change notifications for a set of folders (FSEvents on macOS).
    // Changes collect on a background queue until Poll on the main thread.
    namespace FolderWatcher
    {
        LUMOS_EXPORT bool IsSupported();
        // Replaces the watched set; empty stops watching
        LUMOS_EXPORT void Watch(const TDArray<std::string>& folders);
        // Paths (files or folders) that changed since the last call, deduplicated
        LUMOS_EXPORT u32 Poll(TDArray<std::string>& out);
        LUMOS_EXPORT void Stop();
    }
}
