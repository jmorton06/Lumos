#include "Precompiled.h"
#include "AudioOutput.h"
#include "Maths/MathsUtilities.h"

#ifdef LUMOS_PLATFORM_MACOS
#include <CoreAudio/CoreAudio.h>
#endif

namespace Lumos
{
#ifdef LUMOS_PLATFORM_MACOS
    namespace
    {
        template <typename T>
        bool GetProp(AudioObjectID obj, AudioObjectPropertySelector sel, T& value, AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal)
        {
            AudioObjectPropertyAddress addr = { sel, scope, kAudioObjectPropertyElementMain };
            UInt32 size                     = sizeof(T);
            return AudioObjectGetPropertyData(obj, &addr, 0, nullptr, &size, &value) == noErr;
        }

        std::string CFToString(CFStringRef s)
        {
            if(!s)
                return {};
            char buf[512] = {};
            CFStringGetCString(s, buf, sizeof(buf), kCFStringEncodingUTF8);
            return buf;
        }

        std::string StringProp(AudioObjectID obj, AudioObjectPropertySelector sel)
        {
            CFStringRef s = nullptr;
            if(!GetProp(obj, sel, s) || !s)
                return {};
            std::string out = CFToString(s);
            CFRelease(s);
            return out;
        }

        bool HasOutput(AudioObjectID dev)
        {
            AudioObjectPropertyAddress addr = { kAudioDevicePropertyStreams, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain };
            UInt32 size                     = 0;
            return AudioObjectGetPropertyDataSize(dev, &addr, 0, nullptr, &size) == noErr && size > 0;
        }

        AudioObjectID DefaultOutput()
        {
            AudioObjectID dev = kAudioObjectUnknown;
            GetProp(kAudioObjectSystemObject, kAudioHardwarePropertyDefaultOutputDevice, dev);
            return dev;
        }

        const char* TransportName(UInt32 transport)
        {
            switch(transport)
            {
            case kAudioDeviceTransportTypeBuiltIn: return "builtin";
            case kAudioDeviceTransportTypeUSB: return "usb";
            case kAudioDeviceTransportTypeBluetooth:
            case kAudioDeviceTransportTypeBluetoothLE: return "bluetooth";
            case kAudioDeviceTransportTypeAirPlay: return "airplay";
            case kAudioDeviceTransportTypeHDMI: return "hdmi";
            case kAudioDeviceTransportTypeDisplayPort: return "displayport";
            case kAudioDeviceTransportTypeThunderbolt: return "thunderbolt";
            case kAudioDeviceTransportTypeVirtual:
            case kAudioDeviceTransportTypeAggregate: return "virtual";
            default: return "other";
            }
        }

        void Describe(AudioObjectID dev, AudioOutputInfo& out)
        {
            out           = {};
            out.uid       = StringProp(dev, kAudioDevicePropertyDeviceUID);
            out.name      = StringProp(dev, kAudioObjectPropertyName);
            out.isDefault = dev == DefaultOutput();

            UInt32 transport = 0;
            GetProp(dev, kAudioDevicePropertyTransportType, transport);
            out.transport = TransportName(transport);

            Float64 rate = 0.0;
            if(GetProp(dev, kAudioDevicePropertyNominalSampleRate, rate))
                out.sampleRate = (float)rate;

            AudioObjectPropertyAddress addr = { kAudioDevicePropertyAvailableNominalSampleRates, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
            UInt32 size                     = 0;
            if(AudioObjectGetPropertyDataSize(dev, &addr, 0, nullptr, &size) == noErr && size > 0)
            {
                AudioValueRange ranges[32];
                size = Maths::Min<UInt32>(size, sizeof(ranges));
                if(AudioObjectGetPropertyData(dev, &addr, 0, nullptr, &size, ranges) == noErr)
                {
                    static const float common[] = { 44100, 48000, 88200, 96000, 176400, 192000, 352800, 384000 };
                    for(float r : common)
                        for(u32 i = 0; i < size / sizeof(AudioValueRange); i++)
                            if(r >= ranges[i].mMinimum && r <= ranges[i].mMaximum)
                            {
                                out.sampleRates.PushBack(r);
                                break;
                            }
                }
            }

            // physical format of the first output stream
            addr = { kAudioDevicePropertyStreams, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain };
            AudioStreamID streams[8];
            size = sizeof(streams);
            if(AudioObjectGetPropertyData(dev, &addr, 0, nullptr, &size, streams) == noErr && size >= sizeof(AudioStreamID))
            {
                AudioStreamBasicDescription phys = {};
                if(GetProp(streams[0], kAudioStreamPropertyPhysicalFormat, phys))
                    out.physicalBits = phys.mBitsPerChannel;
            }

            // data sources: AirPlay receivers, internal speakers / headphones
            addr = { kAudioDevicePropertyDataSources, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain };
            if(AudioObjectGetPropertyDataSize(dev, &addr, 0, nullptr, &size) == noErr && size > 0)
            {
                UInt32 ids[64];
                size = Maths::Min<UInt32>(size, sizeof(ids));
                if(AudioObjectGetPropertyData(dev, &addr, 0, nullptr, &size, ids) == noErr)
                {
                    for(u32 i = 0; i < size / sizeof(UInt32); i++)
                    {
                        AudioDeviceSource src;
                        src.id               = ids[i];
                        CFStringRef name     = nullptr;
                        AudioValueTranslation tr = { &src.id, sizeof(UInt32), &name, sizeof(CFStringRef) };
                        AudioObjectPropertyAddress naddr = { kAudioDevicePropertyDataSourceNameForIDCFString, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain };
                        UInt32 tsize = sizeof(tr);
                        if(AudioObjectGetPropertyData(dev, &naddr, 0, nullptr, &tsize, &tr) == noErr && name)
                        {
                            src.name = CFToString(name);
                            CFRelease(name);
                        }
                        out.sources.PushBack(src);
                    }
                }
                UInt32 cur = 0;
                if(GetProp(dev, kAudioDevicePropertyDataSource, cur, kAudioObjectPropertyScopeOutput))
                    out.currentSource = cur;
            }

            pid_t hog = -1;
            if(GetProp(dev, kAudioDevicePropertyHogMode, hog))
                out.hogPid = hog;
            out.ok = true;
        }

        AudioObjectID FromUID(const std::string& uid)
        {
            if(uid.empty())
                return kAudioObjectUnknown;
            CFStringRef cf                  = CFStringCreateWithCString(nullptr, uid.c_str(), kCFStringEncodingUTF8);
            AudioObjectID dev               = kAudioObjectUnknown;
            AudioObjectPropertyAddress addr = { kAudioHardwarePropertyTranslateUIDToDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
            UInt32 size                     = sizeof(dev);
            AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, sizeof(cf), &cf, &size, &dev);
            CFRelease(cf);
            return dev;
        }
    }

    bool GetAudioOutputInfo(AudioOutputInfo& out)
    {
        out               = {};
        AudioObjectID dev = DefaultOutput();
        if(dev == kAudioObjectUnknown)
            return false;
        Describe(dev, out);
        return out.ok;
    }

    bool GetAudioOutputDevices(TDArray<AudioOutputInfo>& out)
    {
        out.Clear();
        AudioObjectPropertyAddress addr = { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
        UInt32 size                     = 0;
        if(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &addr, 0, nullptr, &size) != noErr)
            return false;
        AudioObjectID ids[128];
        size = Maths::Min<UInt32>(size, sizeof(ids));
        if(AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, nullptr, &size, ids) != noErr)
            return false;
        for(u32 i = 0; i < size / sizeof(AudioObjectID); i++)
        {
            if(!HasOutput(ids[i]))
                continue;
            AudioOutputInfo info;
            Describe(ids[i], info);
            if(info.ok)
                out.PushBack(std::move(info));
        }
        return true;
    }

    bool SetAudioOutputSource(const std::string& uid, u32 sourceId)
    {
        AudioObjectID dev = uid.empty() ? DefaultOutput() : FromUID(uid);
        if(dev == kAudioObjectUnknown)
            return false;
        AudioObjectPropertyAddress addr = { kAudioDevicePropertyDataSource, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain };
        return AudioObjectSetPropertyData(dev, &addr, 0, nullptr, sizeof(sourceId), &sourceId) == noErr;
    }

    namespace
    {
        // The elements carrying the device's volume: the main one, else its stereo pair
        u32 VolumeElements(AudioObjectID dev, UInt32 elements[2])
        {
            AudioObjectPropertyAddress addr = { kAudioDevicePropertyVolumeScalar, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain };
            if(AudioObjectHasProperty(dev, &addr))
            {
                elements[0] = kAudioObjectPropertyElementMain;
                return 1;
            }
            UInt32 stereo[2] = { 1, 2 };
            GetProp(dev, kAudioDevicePropertyPreferredChannelsForStereo, stereo, kAudioObjectPropertyScopeOutput);
            u32 n = 0;
            for(u32 i = 0; i < 2; i++)
            {
                addr.mElement = stereo[i];
                if(AudioObjectHasProperty(dev, &addr))
                    elements[n++] = stereo[i];
            }
            return n;
        }
    }

    bool GetAudioOutputVolume(const std::string& uid, float& volume)
    {
        AudioObjectID dev = uid.empty() ? DefaultOutput() : FromUID(uid);
        if(dev == kAudioObjectUnknown)
            return false;
        UInt32 elements[2];
        const u32 n = VolumeElements(dev, elements);
        float loudest = 0.0f;
        bool any      = false;
        for(u32 i = 0; i < n; i++)
        {
            AudioObjectPropertyAddress addr = { kAudioDevicePropertyVolumeScalar, kAudioObjectPropertyScopeOutput, elements[i] };
            Float32 v                       = 0.0f;
            UInt32 size                     = sizeof(v);
            if(AudioObjectGetPropertyData(dev, &addr, 0, nullptr, &size, &v) == noErr)
            {
                loudest = Maths::Max(loudest, (float)v);
                any     = true;
            }
        }
        // muted reads as silent, so the slider sits where the sound is
        UInt32 mute = 0;
        if(any && GetProp(dev, kAudioDevicePropertyMute, mute, kAudioObjectPropertyScopeOutput) && mute)
            loudest = 0.0f;
        volume = loudest;
        return any;
    }

    bool SetAudioOutputVolume(const std::string& uid, float volume)
    {
        AudioObjectID dev = uid.empty() ? DefaultOutput() : FromUID(uid);
        if(dev == kAudioObjectUnknown)
            return false;
        UInt32 elements[2];
        const u32 n = VolumeElements(dev, elements);
        Float32 v   = Maths::Clamp(volume, 0.0f, 1.0f);
        bool any    = false;
        for(u32 i = 0; i < n; i++)
        {
            AudioObjectPropertyAddress addr = { kAudioDevicePropertyVolumeScalar, kAudioObjectPropertyScopeOutput, elements[i] };
            Boolean settable                = false;
            if(AudioObjectIsPropertySettable(dev, &addr, &settable) == noErr && settable)
                any |= AudioObjectSetPropertyData(dev, &addr, 0, nullptr, sizeof(v), &v) == noErr;
        }
        if(any)
        {
            // moving the slider up from silence unmutes, like the volume keys
            AudioObjectPropertyAddress addr = { kAudioDevicePropertyMute, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain };
            Boolean settable                = false;
            UInt32 mute                     = v <= 0.0f ? 1 : 0;
            if(AudioObjectHasProperty(dev, &addr) && AudioObjectIsPropertySettable(dev, &addr, &settable) == noErr && settable)
                AudioObjectSetPropertyData(dev, &addr, 0, nullptr, sizeof(mute), &mute);
        }
        return any;
    }
#elif !defined(LUMOS_PLATFORM_IOS) // iOS: Platform/iOS/iOSAudioSession.mm
    bool GetAudioOutputInfo(AudioOutputInfo& out)
    {
        out = {};
        return false;
    }
    bool GetAudioOutputDevices(TDArray<AudioOutputInfo>& out)
    {
        out.Clear();
        return false;
    }
    bool SetAudioOutputSource(const std::string&, u32) { return false; }
    bool GetAudioOutputVolume(const std::string&, float&) { return false; }
    bool SetAudioOutputVolume(const std::string&, float) { return false; }
#endif

#if !defined(LUMOS_PLATFORM_MACOS) && !defined(LUMOS_PLATFORM_IOS)
    bool ShowAudioRoutePicker(float, float) { return false; }
#endif
}
