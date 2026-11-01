#include "Precompiled.h"
#include "FileActions.h"

// macOS lives in Platform/macOS/MacOSFileActions.mm
#ifndef LUMOS_PLATFORM_MACOS
namespace Lumos
{
    namespace FileActions
    {
        // TODO: SHFileOperation / IFileOperation (Windows), gio trash (Linux)
        bool MoveToTrash(const std::string&) { return false; }
        bool CopyFilesToClipboard(const TDArray<std::string>&) { return false; }
    }
}
#endif
