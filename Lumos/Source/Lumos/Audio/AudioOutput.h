#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    // A selectable input on an output device (e.g. AirPlay receivers, headphone / line out)
    struct AudioDeviceSource
    {
        u32 id = 0;
        std::string name;
    };

    struct AudioOutputInfo
    {
        bool ok = false;
        std::string uid;       // stable id for MusicOutputConfig::deviceUID
        std::string name;
        std::string transport; // builtin, usb, bluetooth, airplay, hdmi, displayport, thunderbolt, virtual, other
        bool isDefault = false;
        float sampleRate = 0.0f;    // device's current nominal rate
        TDArray<float> sampleRates; // rates the device can switch to
        u32 physicalBits = 0;       // bits per sample on the wire (0 = unknown)
        TDArray<AudioDeviceSource> sources;
        u32 currentSource = 0;
        i32 hogPid = -1; // process holding it exclusively
    };

    // What the system is currently sending audio to
    LUMOS_EXPORT bool GetAudioOutputInfo(AudioOutputInfo& out);
    LUMOS_EXPORT bool GetAudioOutputDevices(TDArray<AudioOutputInfo>& out);
    LUMOS_EXPORT bool SetAudioOutputSource(const std::string& uid, u32 sourceId);
    // The device's own volume, 0..1 (uid "" = the system default output). False when the device has
    // no volume control (HDMI, some DACs) or the platform doesn't let apps at it.
    LUMOS_EXPORT bool GetAudioOutputVolume(const std::string& uid, float& volume);
    LUMOS_EXPORT bool SetAudioOutputVolume(const std::string& uid, float volume);
    // The system's AirPlay / output picker (AVRoutePickerView) popping up at a window point in
    // pixels. False where there isn't one.
    LUMOS_EXPORT bool ShowAudioRoutePicker(float x, float y);
}
