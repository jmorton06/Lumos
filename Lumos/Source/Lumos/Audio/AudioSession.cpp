#include "Precompiled.h"
#include "AudioSession.h"

// iOS lives in Platform/iOS/iOSAudioSession.mm
#ifndef LUMOS_PLATFORM_IOS
namespace Lumos
{
    namespace AudioSession
    {
        void SetPlayback(bool) { }
        double RequestSampleRate(double) { return 0.0; }
        double GetSampleRate() { return 0.0; }
        double GetOutputLatency() { return 0.0; }
        std::string GetRouteName() { return {}; }
        bool IsLossyRoute() { return false; }
        void SetOpenALHold(u32, bool) { }
    }
}
#endif
