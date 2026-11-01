#pragma once
#include "Core/Core.h"
#include <string>

namespace Lumos
{
    // How the OS treats the app's sound (iOS): music apps keep playing when the phone locks or the
    // silent switch is on; games stay "ambient" (the default). No-op elsewhere.
    namespace AudioSession
    {
        LUMOS_EXPORT void SetPlayback(bool playback);
        // iOS: asks the hardware to run at this rate (a USB DAC can, the built-in speaker may not);
        // returns the rate it actually runs at. 0 where there's no session.
        LUMOS_EXPORT double RequestSampleRate(double hz);
        LUMOS_EXPORT double GetSampleRate();
        LUMOS_EXPORT double GetOutputLatency();
        // name of what's playing the sound (speaker, headphones, a DAC, AirPods...), and whether that
        // link re-encodes the audio (Bluetooth / AirPlay)
        LUMOS_EXPORT std::string GetRouteName();
        LUMOS_EXPORT bool IsLossyRoute();

        // iOS: OpenAL's output keeps running while its context is current, and iOS takes a running
        // output as "playing" (Control Center shows pause). Holds take the context down; it comes
        // back when none are left. No-op elsewhere.
        enum OpenALHold : u32
        {
            Hold_Interrupted = 1 << 0, // a call / Siri / another app's audio
            Hold_Idle        = 1 << 1, // in the background with nothing playing through OpenAL
        };
        LUMOS_EXPORT void SetOpenALHold(u32 hold, bool on);
    }
}
