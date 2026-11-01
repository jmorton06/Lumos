#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // File-manager style actions on real (physical) paths.
    namespace FileActions
    {
        // Moves to the Trash / Recycle Bin; false when unsupported or it failed
        LUMOS_EXPORT bool MoveToTrash(const std::string& path);
        // Puts files on the clipboard so they paste into the file manager
        LUMOS_EXPORT bool CopyFilesToClipboard(const TDArray<std::string>& paths);
    }
}
