#include "Precompiled.h"
#include "Audio/ChainDecoder.h"
#include "Audio/Equaliser.h"
#include "Audio/TimeStretch.h"
#include "MacOSMusicStream.h"
#include "Audio/AudioDecoder.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/MathsUtilities.h"

#include <TargetConditionals.h>
#include <AudioToolbox/AudioToolbox.h>
#if TARGET_OS_IPHONE
#include "Audio/AudioSession.h"
#else
#include <CoreAudio/CoreAudio.h>
#endif
#include <unistd.h>
#include <thread>
#include <cstring>

namespace Lumos
{
    namespace
    {
#if !TARGET_OS_IPHONE
        template <typename T>
        bool GetProp(AudioObjectID obj, AudioObjectPropertySelector sel, T& value, AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal)
        {
            AudioObjectPropertyAddress addr = { sel, scope, kAudioObjectPropertyElementMain };
            UInt32 size                     = sizeof(T);
            return AudioObjectGetPropertyData(obj, &addr, 0, nullptr, &size, &value) == noErr;
        }

        template <typename T>
        bool SetProp(AudioObjectID obj, AudioObjectPropertySelector sel, const T& value, AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal)
        {
            AudioObjectPropertyAddress addr = { sel, scope, kAudioObjectPropertyElementMain };
            return AudioObjectSetPropertyData(obj, &addr, 0, nullptr, sizeof(T), &value) == noErr;
        }

        AudioObjectID DeviceFromUID(const std::string& uid)
        {
            AudioObjectID dev = kAudioObjectUnknown;
            if(uid.empty())
            {
                GetProp(kAudioObjectSystemObject, kAudioHardwarePropertyDefaultOutputDevice, dev);
                return dev;
            }
            CFStringRef cf                  = CFStringCreateWithCString(nullptr, uid.c_str(), kCFStringEncodingUTF8);
            AudioObjectPropertyAddress addr = { kAudioHardwarePropertyTranslateUIDToDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
            UInt32 size                     = sizeof(dev);
            AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, sizeof(cf), &cf, &size, &dev);
            CFRelease(cf);
            return dev;
        }

        bool RateSupported(AudioObjectID dev, double rate)
        {
            AudioObjectPropertyAddress addr = { kAudioDevicePropertyAvailableNominalSampleRates, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
            UInt32 size                     = 0;
            if(AudioObjectGetPropertyDataSize(dev, &addr, 0, nullptr, &size) != noErr || size == 0)
                return false;
            AudioValueRange ranges[32];
            size = Maths::Min<UInt32>(size, sizeof(ranges));
            if(AudioObjectGetPropertyData(dev, &addr, 0, nullptr, &size, ranges) != noErr)
                return false;
            for(u32 i = 0; i < size / sizeof(AudioValueRange); i++)
                if(rate >= ranges[i].mMinimum && rate <= ranges[i].mMaximum)
                    return true;
            return false;
        }

        // Devices we changed, so they can be handed back as we found them
        struct DeviceState
        {
            AudioObjectID dev;
            Float64 originalRate;
            bool rateChanged;
            bool hogged;
            bool formatChanged;
            AudioStreamID stream;
            AudioStreamBasicDescription originalFormat;
        };

#endif

        struct Registry
        {
            Mutex Lock;
            TDArray<MacOSMusicStream*> Streams;
#if !TARGET_OS_IPHONE
            TDArray<DeviceState> Devices;
#endif
            std::thread Thread;
            std::atomic<bool> Running { false };
            bool Initialised = false;
        };

        Registry& Reg()
        {
            static Registry s_Reg;
            if(!s_Reg.Initialised)
            {
                MutexInit(&s_Reg.Lock);
                MutexSetName(&s_Reg.Lock, "NativeMusicStreams");
                s_Reg.Initialised = true;
            }
            return s_Reg;
        }

#if !TARGET_OS_IPHONE
        DeviceState& StateFor(Registry& reg, AudioObjectID dev)
        {
            for(auto& d : reg.Devices)
                if(d.dev == dev)
                    return d;
            DeviceState st = { dev, 0.0, false, false, false, 0, {} };
            GetProp(dev, kAudioDevicePropertyNominalSampleRate, st.originalRate);
            reg.Devices.PushBack(st);
            return reg.Devices.Back();
        }
#endif

        void ThreadMain()
        {
            auto& reg = Reg();
            while(reg.Running.load())
            {
                {
                    ScopedMutex lock(&reg.Lock);
                    for(auto* s : reg.Streams)
                        s->Update();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        OSStatus RenderCallback(void* ref, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32 frames, AudioBufferList* io)
        {
            if(io && io->mNumberBuffers > 0)
                static_cast<MacOSMusicStream*>(ref)->RenderInto((float*)io->mBuffers[0].mData, frames);
            return noErr;
        }
    }

    MacOSMusicStream::MacOSMusicStream(const std::string& path, AudioDecoder* decoder, const MusicOutputConfig& config)
        : m_Decoder(decoder)
        , m_Config(config)
    {
        MutexInit(&m_Lock);
        m_Path       = path;
        m_SampleRate = decoder->GetSampleRate();
        m_Channels   = Maths::Clamp(decoder->GetChannels(), 1u, 2u);
        m_Duration   = decoder->GetDuration();
        m_DoP        = decoder->IsDoP();
        auto& reg    = Reg();

#if TARGET_OS_IPHONE
        // one route at a time; the hardware runs at the file's rate when it can (USB DACs do)
        m_DeviceName  = AudioSession::GetRouteName();
        m_LossyLink   = AudioSession::IsLossyRoute();
        m_DeviceRate  = (u32)(config.matchRate ? AudioSession::RequestSampleRate(m_SampleRate) : AudioSession::GetSampleRate());
        m_RateMatched = m_DeviceRate == m_SampleRate;
        m_Latency     = AudioSession::GetOutputLatency();
        if(m_DoP)
            return; // no exclusive access on iOS
#else
        AudioObjectID dev = DeviceFromUID(config.deviceUID);
        if(dev == kAudioObjectUnknown)
            dev = DeviceFromUID("");
        m_Device = dev;

        CFStringRef name = nullptr;
        if(GetProp(dev, kAudioObjectPropertyName, name) && name)
        {
            char buf[256] = {};
            CFStringGetCString(name, buf, sizeof(buf), kCFStringEncodingUTF8);
            m_DeviceName = buf;
            CFRelease(name);
        }

        UInt32 transport = 0;
        GetProp(dev, kAudioDevicePropertyTransportType, transport);
        m_LossyLink = transport == kAudioDeviceTransportTypeBluetooth || transport == kAudioDeviceTransportTypeBluetoothLE
                      || transport == kAudioDeviceTransportTypeAirPlay;

        {
            ScopedMutex lock(&reg.Lock);
            DeviceState& st = StateFor(reg, dev);
            if(config.exclusive)
            {
                pid_t hog = -1;
                GetProp(dev, kAudioDevicePropertyHogMode, hog);
                if(hog == -1)
                {
                    pid_t me = getpid();
                    if(SetProp(dev, kAudioDevicePropertyHogMode, me))
                        st.hogged = true;
                }
                GetProp(dev, kAudioDevicePropertyHogMode, hog);
                m_Hogged = hog == getpid();
            }
            if((config.matchRate || m_DoP) && RateSupported(dev, m_SampleRate))
            {
                Float64 now = 0.0;
                GetProp(dev, kAudioDevicePropertyNominalSampleRate, now);
                if((u32)now != m_SampleRate)
                {
                    Float64 want = m_SampleRate;
                    if(SetProp(dev, kAudioDevicePropertyNominalSampleRate, want))
                    {
                        st.rateChanged = true;
                        // the switch is asynchronous
                        for(int i = 0; i < 50 && (u32)now != m_SampleRate; i++)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            GetProp(dev, kAudioDevicePropertyNominalSampleRate, now);
                        }
                    }
                }
            }
        }

        Float64 rate = 0.0;
        GetProp(dev, kAudioDevicePropertyNominalSampleRate, rate);
        m_DeviceRate  = (u32)rate;
        m_RateMatched = m_DeviceRate == m_SampleRate;

        AudioObjectPropertyAddress saddr = { kAudioDevicePropertyStreams, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain };
        AudioStreamID streams[8];
        UInt32 ssize = sizeof(streams);
        if(AudioObjectGetPropertyData(dev, &saddr, 0, nullptr, &ssize, streams) == noErr && ssize >= sizeof(AudioStreamID))
        {
            AudioStreamBasicDescription phys = {};
            // exclusive: run the device at the file's own bit depth (integer PCM, same rate)
            const u32 srcBits = m_DoP ? 24u : decoder->GetSourceBits(); // DoP words are 24-bit
            if(m_Hogged && (config.matchBits || m_DoP) && srcBits > 0 && GetProp(streams[0], kAudioStreamPropertyPhysicalFormat, phys)
               && phys.mBitsPerChannel != srcBits)
            {
                AudioObjectPropertyAddress faddr = { kAudioStreamPropertyAvailablePhysicalFormats, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
                UInt32 fsize = 0;
                if(AudioObjectGetPropertyDataSize(streams[0], &faddr, 0, nullptr, &fsize) == noErr && fsize > 0)
                {
                    TDArray<AudioStreamRangedDescription> fmts;
                    fmts.Resize(fsize / sizeof(AudioStreamRangedDescription));
                    AudioObjectGetPropertyData(streams[0], &faddr, 0, nullptr, &fsize, fmts.Data());
                    const AudioStreamBasicDescription* best = nullptr;
                    for(auto& f : fmts)
                    {
                        const auto& d = f.mFormat;
                        if(d.mFormatID != kAudioFormatLinearPCM || (d.mFormatFlags & kAudioFormatFlagIsFloat) || d.mChannelsPerFrame != phys.mChannelsPerFrame)
                            continue;
                        if(m_SampleRate < f.mSampleRateRange.mMinimum || m_SampleRate > f.mSampleRateRange.mMaximum)
                            continue;
                        if(d.mBitsPerChannel < srcBits)
                            continue;
                        if(!best || d.mBitsPerChannel < best->mBitsPerChannel)
                            best = &d; // the smallest that holds every bit
                    }
                    if(best && best->mBitsPerChannel != phys.mBitsPerChannel)
                    {
                        AudioStreamBasicDescription want = *best;
                        want.mSampleRate                 = m_SampleRate;
                        ScopedMutex lock(&reg.Lock);
                        DeviceState& st = StateFor(reg, dev);
                        if(!st.formatChanged)
                        {
                            st.originalFormat = phys;
                            st.stream         = streams[0];
                        }
                        if(SetProp(streams[0], kAudioStreamPropertyPhysicalFormat, want))
                            st.formatChanged = true;
                    }
                }
            }
            if(GetProp(streams[0], kAudioStreamPropertyPhysicalFormat, phys))
                m_DeviceBits = phys.mBitsPerChannel;
            UInt32 dl = 0, sl = 0, so = 0, buf = 0;
            GetProp(dev, kAudioDevicePropertyLatency, dl, kAudioObjectPropertyScopeOutput);
            GetProp(dev, kAudioDevicePropertySafetyOffset, so, kAudioObjectPropertyScopeOutput);
            GetProp(dev, kAudioDevicePropertyBufferFrameSize, buf);
            GetProp(streams[0], kAudioStreamPropertyLatency, sl);
            if(m_DeviceRate > 0)
                m_Latency = (double)(dl + sl + so + buf) / m_DeviceRate;
        }

        // DoP only works bit for bit: the device ours alone, at the DoP rate, holding 24 bits
        if(m_DoP && !(m_Hogged && m_RateMatched && m_DeviceBits >= 24))
        {
            LINFO("MacOSMusicStream: %s can't take DoP here (exclusive %d, %u Hz, %u-bit)", m_DeviceName.c_str(), (int)m_Hogged, m_DeviceRate, m_DeviceBits);
            return;
        }
#endif

        // ~0.5s of stereo float, rounded up to a power of two
        m_RingFrames = 16384;
        while(m_RingFrames < m_SampleRate / 2)
            m_RingFrames <<= 1;
        m_Ring    = new float[m_RingFrames * 2]();
        m_Scratch = new float[m_ScratchFrames * m_Channels];

#if TARGET_OS_IPHONE
        AudioComponentDescription desc = { kAudioUnitType_Output, kAudioUnitSubType_RemoteIO, kAudioUnitManufacturer_Apple, 0, 0 };
#else
        AudioComponentDescription desc = { kAudioUnitType_Output, kAudioUnitSubType_HALOutput, kAudioUnitManufacturer_Apple, 0, 0 };
#endif
        AudioComponent comp            = AudioComponentFindNext(nullptr, &desc);
        AudioComponentInstance unit    = nullptr;
        if(!comp || AudioComponentInstanceNew(comp, &unit) != noErr)
        {
            LWARN("MacOSMusicStream: no HAL output unit");
            return;
        }

        AudioStreamBasicDescription fmt = {};
        fmt.mSampleRate                 = m_SampleRate;
        fmt.mFormatID                   = kAudioFormatLinearPCM;
        fmt.mFormatFlags                = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        fmt.mChannelsPerFrame           = 2;
        fmt.mBitsPerChannel             = 32;
        fmt.mBytesPerFrame              = 8;
        fmt.mFramesPerPacket            = 1;
        fmt.mBytesPerPacket             = 8;
        AURenderCallbackStruct cb       = { RenderCallback, this };

#if !TARGET_OS_IPHONE
        AudioObjectID devId = dev;
        if(AudioUnitSetProperty(unit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &devId, sizeof(devId)) != noErr)
        {
            LWARN("MacOSMusicStream: couldn't open %s", m_DeviceName.c_str());
            AudioComponentInstanceDispose(unit);
            return;
        }
#endif
        if(AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &fmt, sizeof(fmt)) != noErr
           || AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cb, sizeof(cb)) != noErr
           || AudioUnitInitialize(unit) != noErr)
        {
            LWARN("MacOSMusicStream: couldn't open %s", m_DeviceName.c_str());
            AudioComponentInstanceDispose(unit);
            return;
        }
        m_Unit = unit;

        {
            ScopedMutex lock(&reg.Lock);
            reg.Streams.PushBack(this);
        }
        if(!reg.Running.load())
        {
            reg.Running = true;
            reg.Thread  = std::thread(ThreadMain);
        }
    }

    MacOSMusicStream::~MacOSMusicStream()
    {
        {
            auto& reg = Reg();
            ScopedMutex lock(&reg.Lock);
            for(u32 i = 0; i < reg.Streams.Size(); i++)
                if(reg.Streams[i] == this)
                {
                    reg.Streams[i] = reg.Streams.Back();
                    reg.Streams.PopBack();
                    break;
                }
        }
        Release();
        delete m_Decoder;
        delete[] m_Ring;
        delete[] m_Scratch;
        MutexDestroy(&m_Lock);
    }

    void MacOSMusicStream::Release()
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released)
            return;
        m_Released = true;
        if(m_Unit)
        {
            auto unit = (AudioComponentInstance)m_Unit;
            AudioOutputUnitStop(unit);
            AudioUnitUninitialize(unit);
            AudioComponentInstanceDispose(unit);
            m_Unit    = nullptr;
            m_Running = false;
        }
    }

    void MacOSMusicStream::RestoreDevices(bool rates, bool hog)
    {
#if TARGET_OS_IPHONE
        (void)rates;
        (void)hog;
#else
        auto& reg = Reg();
        ScopedMutex lock(&reg.Lock);
        for(auto& d : reg.Devices)
        {
            if(rates && d.rateChanged && d.originalRate > 0.0)
            {
                SetProp(d.dev, kAudioDevicePropertyNominalSampleRate, d.originalRate);
                d.rateChanged = false;
            }
            if(hog && d.formatChanged)
            {
                SetProp(d.stream, kAudioStreamPropertyPhysicalFormat, d.originalFormat);
                d.formatChanged = false;
            }
            if(hog && d.hogged)
            {
                pid_t none = -1;
                SetProp(d.dev, kAudioDevicePropertyHogMode, none);
                d.hogged = false;
            }
        }
#endif
    }

    void MacOSMusicStream::ShutdownAll()
    {
        auto& reg = Reg();
        if(reg.Running.load())
        {
            reg.Running = false;
            if(reg.Thread.joinable())
                reg.Thread.join();
        }
        {
            ScopedMutex lock(&reg.Lock);
            for(auto* s : reg.Streams)
                s->Release();
        }
        RestoreDevices(true, true);
    }

    // ---------------------------------------------------------------- IO

    void MacOSMusicStream::RenderInto(float* out, u32 frames)
    {
        u64 read  = m_Read.load(std::memory_order_relaxed);
        u64 write = m_Write.load(std::memory_order_acquire);
        u32 n     = (u32)Maths::Min<u64>(frames, write - read);
        for(u32 i = 0; i < n; i++)
        {
            const float* src = m_Ring + ((read + i) & (m_RingFrames - 1)) * 2;
            out[i * 2]       = src[0];
            out[i * 2 + 1]   = src[1];
        }
        if(n < frames)
            memset(out + n * 2, 0, (frames - n) * 2 * sizeof(float));

        if(m_DoP)
        {
            // DoP words go out exactly as decoded; the DAC does the volume
            m_Dithering = false;
            m_Read.store(read + n, std::memory_order_release);
            return;
        }

        // unity gain leaves the samples untouched; changes ramp over the buffer
        float target = m_Gain.load(std::memory_order_relaxed);
        if(target != 1.0f || m_AppliedGain != 1.0f)
        {
            float g    = m_AppliedGain;
            float step = n > 0 ? (target - g) / n : 0.0f;
            for(u32 i = 0; i < n; i++)
            {
                g += step;
                out[i * 2] *= g;
                out[i * 2 + 1] *= g;
            }
            m_AppliedGain = target;
        }
        // TPDF dither at the device's least significant bit when samples no longer land exactly on
        // it (EQ, speed, volume, or a deeper / lossy source). Untouched samples are left alone.
        const u32 bits  = m_DeviceBits;
        const bool deep = m_Decoder && (m_Decoder->GetSourceBits() == 0 || m_Decoder->GetSourceBits() > bits);
        m_Dithering     = m_Config.dither && bits >= 8 && bits <= 24 && (deep || target != 1.0f || m_Processed.load(std::memory_order_relaxed));
        if(m_Dithering)
        {
            const float lsb = 1.0f / (float)(1u << (bits - 1));
            u32 x           = m_DitherSeed;
            for(u32 i = 0; i < n * 2; i++)
            {
                x ^= x << 13; x ^= x >> 17; x ^= x << 5;
                const float r1 = (float)(x & 0xFFFF) / 65536.0f;
                x ^= x << 13; x ^= x >> 17; x ^= x << 5;
                const float r2 = (float)(x & 0xFFFF) / 65536.0f;
                out[i] += (r1 - r2) * lsb;
            }
            m_DitherSeed = x;
        }
        m_Read.store(read + n, std::memory_order_release);
    }

    void MacOSMusicStream::Fill()
    {
        // caller holds m_Lock
        while(!m_EndOfFile.load())
        {
            u64 write = m_Write.load(std::memory_order_relaxed);
            u64 read  = m_Read.load(std::memory_order_acquire);
            u64 space = m_RingFrames - (write - read);
            if(space < m_ScratchFrames)
                break;
            m_Processed.store(Equaliser::IsActive() || SoundChain::HasActiveEffects() || (m_Stretch && m_Stretch->GetSpeed() != 1.0f) || GetVarispeed() != 1.0f, std::memory_order_relaxed);
            u32 got = m_Decoder->ReadFloat(m_Scratch, m_ScratchFrames);
            if(got == 0)
            {
                if(m_Looping && m_Decoder->GetTotalFrames() > 0 && m_Decoder->Seek(0))
                    got = m_Decoder->ReadFloat(m_Scratch, m_ScratchFrames);
                if(got == 0)
                {
                    m_EndOfFile = true;
                    break;
                }
            }
            const u32 ch = m_Channels;
            for(u32 i = 0; i < got; i++)
            {
                float* dst = m_Ring + ((write + i) & (m_RingFrames - 1)) * 2;
                dst[0]     = m_Scratch[i * ch];
                dst[1]     = m_Scratch[i * ch + (ch > 1 ? 1 : 0)];
            }
            ScopePushF32(m_Scratch, got, ch, write);
            m_Write.store(write + got, std::memory_order_release);
        }
    }

    void MacOSMusicStream::Restart(u64 frame)
    {
        m_Decoder->Seek(frame);
        m_Read.store(frame);
        m_Write.store(frame);
        m_SegmentStart = frame;
        m_EndOfFile    = false;
        m_DrainedAt    = -1.0;
        ScopeReset(frame);
        Fill();
    }

    void MacOSMusicStream::Update()
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released || !m_WantPlaying || m_Paused)
            return;
        Fill();
        if(m_EndOfFile.load() && m_Read.load() >= m_Write.load())
        {
            // drained: let the device play out what it has buffered, then stop
            double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
            if(m_DrainedAt < 0.0)
                m_DrainedAt = now;
            if(now - m_DrainedAt < m_Latency + 0.05)
                return;
            if(m_Running)
            {
                AudioOutputUnitStop((AudioComponentInstance)m_Unit);
                m_Running = false;
            }
            m_WantPlaying = false;
            m_DrainedAt   = -1.0;
        }
    }

    // ---------------------------------------------------------------- transport

    void MacOSMusicStream::Play()
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released || !m_Unit)
            return;
        if(m_Paused)
            m_Paused = false;
        else if(m_WantPlaying)
            return;
        else if(m_EndOfFile.load() || m_Write.load() == m_Read.load())
            Restart(m_EndOfFile.load() ? 0 : m_Read.load());
        m_WantPlaying = true;
        if(!m_Running && AudioOutputUnitStart((AudioComponentInstance)m_Unit) == noErr)
            m_Running = true;
    }

    void MacOSMusicStream::Pause()
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released || !m_WantPlaying || m_Paused)
            return;
        m_Paused = true;
        if(m_Running)
        {
            AudioOutputUnitStop((AudioComponentInstance)m_Unit);
            m_Running = false;
        }
    }

    void MacOSMusicStream::Stop()
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released)
            return;
        if(m_Running)
        {
            AudioOutputUnitStop((AudioComponentInstance)m_Unit);
            m_Running = false;
        }
        m_WantPlaying = false;
        m_Paused      = false;
        Restart(0);
    }

    void MacOSMusicStream::Seek(double seconds)
    {
        ScopedMutex lock(&m_Lock);
        if(m_Released)
            return;
        u64 frame = (u64)(Maths::Max(0.0, seconds) * m_SampleRate);
        u64 total = m_Chain ? m_Chain->GetTotalFrames() : m_Decoder->GetTotalFrames();
        if(total > 0 && frame >= total)
            frame = total - 1;
        bool wasRunning = m_Running;
        if(wasRunning)
            AudioOutputUnitStop((AudioComponentInstance)m_Unit);
        Restart(frame);
        if(wasRunning)
            AudioOutputUnitStart((AudioComponentInstance)m_Unit);
    }

    u64 MacOSMusicStream::LatencyFrames() const { return (u64)(m_Latency * m_SampleRate); }

    u64 MacOSMusicStream::ScopePlayFrame() const
    {
        u64 read = m_Read.load();
        u64 lat  = LatencyFrames();
        if(m_DrainedAt >= 0.0)
        {
            // nothing new is being read; what the device buffered is still coming out
            double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
            u64 played = (u64)((now - m_DrainedAt) * m_SampleRate);
            lat        = lat > played ? lat - played : 0;
        }
        return read > m_SegmentStart + lat ? read - lat : m_SegmentStart;
    }

    bool MacOSMusicStream::ScopeActive() const
    {
        ScopedMutex lock(&m_Lock);
        return m_WantPlaying;
    }

    u64 MacOSMusicStream::PlayedSourceFrame() const
    {
        ScopedMutex lock(&m_Lock);
        return m_Decoder->SourceFrame(ScopePlayFrame());
    }

    double MacOSMusicStream::GetPosition() const
    {
        ScopedMutex lock(&m_Lock);
        u64 frame = m_Decoder->SourceFrame(ScopePlayFrame());
        if(m_Chain)
            frame = LocalFrame(frame, nullptr);
        else
        {
            u64 total = m_Decoder->GetTotalFrames();
            if(total > 0)
                frame = m_Looping ? frame % total : Maths::Min(frame, total);
        }
        return m_SampleRate ? (double)frame / m_SampleRate : 0.0;
    }

    bool MacOSMusicStream::IsPlaying() const
    {
        ScopedMutex lock(&m_Lock);
        return m_WantPlaying && !m_Paused;
    }

    bool MacOSMusicStream::IsPaused() const
    {
        ScopedMutex lock(&m_Lock);
        return m_Paused;
    }

    bool MacOSMusicStream::IsFinished() const
    {
        ScopedMutex lock(&m_Lock);
        return !m_WantPlaying && m_EndOfFile.load() && m_Read.load() >= m_Write.load();
    }

    void MacOSMusicStream::SetVolume(float volume)
    {
        m_Volume = Maths::Max(0.0f, volume);
        m_Gain.store(m_Volume);
    }

    MusicOutputStatus MacOSMusicStream::GetOutputStatus() const
    {
        MusicOutputStatus st;
        st.backend     = "coreaudio";
        st.deviceName  = m_DeviceName;
        st.fileRate    = m_SampleRate;
        st.fileBits    = m_Decoder->GetSourceBits();
        st.deviceRate  = m_DeviceRate;
        st.deviceBits  = m_DeviceBits;
        st.floatPath   = true;
        st.rateMatched = m_RateMatched;
        st.exclusive   = m_Hogged;
        st.dithering   = m_Dithering;
        st.dop         = m_DoP;
        st.unityGain   = m_Gain.load() == 1.0f;
        st.lossyLink   = m_LossyLink;
        st.latency     = m_Latency;
        return st;
    }
}
