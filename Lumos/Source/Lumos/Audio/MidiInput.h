#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // MIDI keyboards / controllers (CoreMIDI on macOS). Every connected source is listened to;
    // events queue up until Poll on the main thread.
    namespace MidiInput
    {
        enum class EventType : u8
        {
            NoteOn,
            NoteOff,
            Control, // CC: number = controller, value = 0..127
            PitchBend, // value = -8192..8191
        };

        struct Event
        {
            EventType type;
            u8 channel = 0;
            u8 number  = 0; // note / controller
            i32 value  = 0; // velocity / cc value / bend
            double time = 0.0; // seconds, steady clock
        };

        struct Device
        {
            std::string name;
            bool online = true;
        };

        LUMOS_EXPORT bool IsSupported();
        // Starts listening (idempotent); picks up devices plugged in later
        LUMOS_EXPORT bool Start();
        LUMOS_EXPORT void Stop();
        LUMOS_EXPORT void GetDevices(TDArray<Device>& out);
        LUMOS_EXPORT u32 Poll(Event* out, u32 max);
        // Feed events from elsewhere (tests, an on-screen / computer keyboard)
        LUMOS_EXPORT void Inject(const Event& e);
        LUMOS_EXPORT void InjectBytes(const u8* data, size_t size); // raw MIDI stream
    }
}
