#pragma once
#include "AudioTags.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // Reads tags on worker threads (library scans: a slow file must never cost a frame).
    namespace AudioTagReader
    {
        struct Result
        {
            u32 id = 0;
            std::string path;
            bool ok = false;
            AudioTags tags; // no artwork bytes
        };

        LUMOS_EXPORT u32 Queue(const std::string& path);
        // Finished results since the last call (at most max)
        LUMOS_EXPORT u32 Poll(TDArray<Result>& out, u32 max);
        LUMOS_EXPORT u32 Pending();
        LUMOS_EXPORT void Clear();
        LUMOS_EXPORT void Shutdown();
    }
}
