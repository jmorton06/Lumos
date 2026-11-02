#include "Precompiled.h"
#include "Bookmarks.h"

// macOS / iOS live in Platform/MacOS/MacOSBookmarks.mm
#if !defined(LUMOS_PLATFORM_MACOS) && !defined(LUMOS_PLATFORM_IOS)
namespace Lumos
{
    std::string Bookmarks::Create(const std::string&) { return std::string(); }
    std::string Bookmarks::Resolve(const std::string&, bool* stale)
    {
        if(stale)
            *stale = false;
        return std::string();
    }
}
#endif
