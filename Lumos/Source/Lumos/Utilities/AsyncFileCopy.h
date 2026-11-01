#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // Copies files on a worker thread, creating destination folders (library import / backup).
    namespace AsyncFileCopy
    {
        struct Result
        {
            u32 id = 0;
            std::string src, dst, error; // error empty on success
            u64 bytes = 0;
        };

        LUMOS_EXPORT u32 Queue(const std::string& src, const std::string& dst, bool overwrite = false);
        LUMOS_EXPORT u32 Poll(TDArray<Result>& out, u32 max);
        LUMOS_EXPORT u32 Pending();
        LUMOS_EXPORT void Clear(); // drops queued copies (one already running finishes)
        LUMOS_EXPORT void Shutdown();
    }
}
