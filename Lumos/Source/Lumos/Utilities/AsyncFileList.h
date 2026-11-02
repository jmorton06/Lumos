#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // Folder listing (sorted paths, dot-files skipped, optional extension filter). Big folders take
    // 100ms+ to walk, so Queue / Poll do it on a worker thread.
    namespace AsyncFileList
    {
        LUMOS_EXPORT void ParseExtensions(const std::string& csv, TDArray<std::string>& out);
        LUMOS_EXPORT void List(const std::string& root, bool recursive, const TDArray<std::string>& exts, TDArray<std::string>& out);
        LUMOS_EXPORT u32 Queue(const std::string& root, bool recursive, const std::string& extensionsCsv);
        // Checks the paths on the worker (a stalled network drive mustn't freeze a frame); Poll gives the missing ones
        LUMOS_EXPORT u32 QueueMissing(TDArray<std::string>&& paths);
        // True once the listing is ready (out filled); false while still walking or for an unknown id
        LUMOS_EXPORT bool Poll(u32 id, TDArray<std::string>& out);
        LUMOS_EXPORT void Shutdown();
    }
}
