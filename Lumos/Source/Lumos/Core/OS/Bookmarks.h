#pragma once
#include "Core/Core.h"
#include <string>

namespace Lumos
{
    // Lasting access to folders the user picked. A sandboxed app (Mac App Store, iOS) loses access
    // to a picked folder when it quits unless it keeps a security-scoped bookmark and resolves it
    // on the next launch. Elsewhere these are no-ops.
    namespace Bookmarks
    {
        // Opaque bookmark (base64) for a path the app can reach now; "" if unsupported / failed
        LUMOS_EXPORT std::string Create(const std::string& path);
        // Path the bookmark points at now, with access started for the rest of the run; "" if it
        // can't be resolved. stale = the bookmark should be created again.
        LUMOS_EXPORT std::string Resolve(const std::string& bookmark, bool* stale = nullptr);
    }
}
