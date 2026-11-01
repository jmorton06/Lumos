#include "Precompiled.h"
#include "NowPlaying.h"

// macOS and iOS live in Platform/MacOS/MacOSNowPlaying.mm
#if !defined(LUMOS_PLATFORM_MACOS) && !defined(LUMOS_PLATFORM_IOS)
namespace Lumos
{
    namespace NowPlaying
    {
        bool IsSupported() { return false; }
        void Set(const Info&) { }
        void Clear() { }
        u32 Poll(Command*, u32) { return 0; }
        void Inject(CommandType, double) { }
        bool GetCurrent(Info&, bool&) { return false; }
        void SetCommandHook(void (*)()) { }
    }
}
#endif
