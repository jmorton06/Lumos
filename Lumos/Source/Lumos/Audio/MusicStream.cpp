#include "Precompiled.h"
#include "MusicStream.h"
#include "RemoteAudio.h"
#include "ChainDecoder.h"
#include "Equaliser.h"
#include "Audio/TimeStretch.h"
#include "Audio/Varispeed.h"
#include "AudioDecoder.h"
#include "DSDDecoder.h"
#include "Utilities/StringUtilities.h"
#include "AudioAnalysis.h"
#include "Maths/MathsUtilities.h"

#include <cmath>

#ifdef LUMOS_OPENAL
#include "Platform/OpenAL/ALMusicStream.h"
#endif
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
#define LUMOS_NATIVE_MUSIC 1 // Core Audio output (AUHAL on macOS, RemoteIO on iOS)
#include "Platform/MacOS/MacOSMusicStream.h"
#endif

namespace Lumos
{
    namespace
    {
        MusicOutputConfig DefaultConfig()
        {
            MusicOutputConfig c;
#ifdef LUMOS_PLATFORM_IOS
            // RemoteIO stops when paused, so iOS shows the app as paused (OpenAL's output never stops)
            c.native = true;
#endif
            return c;
        }
        MusicOutputConfig s_Config = DefaultConfig();

        bool WantsNative(const MusicOutputConfig& c)
        {
            return c.native || c.matchRate || c.exclusive || !c.deviceUID.empty();
        }

        // In-place radix-2 FFT, n must be a power of two
        void FFT(float* re, float* im, u32 n)
        {
            for(u32 i = 1, j = 0; i < n; i++)
            {
                u32 bit = n >> 1;
                for(; j & bit; bit >>= 1)
                    j ^= bit;
                j ^= bit;
                if(i < j)
                {
                    float t = re[i];
                    re[i]   = re[j];
                    re[j]   = t;
                    t       = im[i];
                    im[i]   = im[j];
                    im[j]   = t;
                }
            }
            for(u32 len = 2; len <= n; len <<= 1)
            {
                float ang = -2.0f * Maths::M_PI / (float)len;
                float wr = cosf(ang), wi = sinf(ang);
                for(u32 i = 0; i < n; i += len)
                {
                    float cr = 1.0f, ci = 0.0f;
                    for(u32 k = 0; k < len / 2; k++)
                    {
                        u32 a = i + k, b = i + k + len / 2;
                        float tr = re[b] * cr - im[b] * ci;
                        float ti = re[b] * ci + im[b] * cr;
                        re[b]    = re[a] - tr;
                        im[b]    = im[a] - ti;
                        re[a] += tr;
                        im[a] += ti;
                        float nr = cr * wr - ci * wi;
                        ci       = cr * wi + ci * wr;
                        cr       = nr;
                    }
                }
            }
        }
    }

    void MusicStream::SetSpeed(float speed)
    {
        if(m_Stretch)
            m_Stretch->SetSpeed(speed);
    }

    bool MusicStream::QueueNext(const std::string& path)
    {
        if(!m_Chain)
            return false;
        AudioDecoder* d = nullptr;
        if(m_Chain->IsDoP() && DSDDecoder::IsDSDExtension(StringUtilities::GetFilePathExtension(path)))
            d = DSDDecoder::Open(path, true); // stays DoP so the join is seamless
        else
            d = m_PreciseDecoders ? AudioDecoder::OpenPrecise(path) : AudioDecoder::Open(path);
        return d && m_Chain->Queue(d);
    }

    void MusicStream::ClearNext()
    {
        if(m_Chain)
            m_Chain->ClearQueue();
    }

    u32 MusicStream::GetTrackIndex() const
    {
        if(!m_Chain)
            return 0;
        u32 index = 0;
        m_Chain->Locate(PlayedSourceFrame(), &index, nullptr, nullptr);
        return index;
    }

    u64 MusicStream::LocalFrame(u64 frame, u64* total) const
    {
        if(m_Chain)
        {
            u64 local = 0, t = 0;
            m_Chain->Locate(frame, nullptr, &local, &t);
            if(total)
                *total = t;
            return t > 0 ? Maths::Min(local, t) : local;
        }
        if(total)
            *total = 0;
        return frame;
    }

    double MusicStream::GetDuration() const
    {
        if(!m_Chain || !m_SampleRate)
            return m_Duration;
        u64 total = 0;
        m_Chain->Locate(PlayedSourceFrame(), nullptr, nullptr, &total);
        return total > 0 ? (double)total / m_SampleRate : m_Duration;
    }

    void MusicStream::SetVarispeed(float rate)
    {
        if(m_Varispeed)
            m_Varispeed->SetRate(rate);
    }

    float MusicStream::GetVarispeed() const
    {
        return m_Varispeed ? m_Varispeed->GetRate() : 1.0f;
    }

    float MusicStream::GetSpeed() const
    {
        return m_Stretch ? m_Stretch->GetSpeed() : 1.0f;
    }

    SharedPtr<MusicStream> MusicStream::Create(const std::string& path)
    {
#ifdef LUMOS_NATIVE_MUSIC
        // DSD straight to a DAC that decodes it; falls through to PCM when the device can't take DoP
        if(s_Config.dop && s_Config.exclusive && DSDDecoder::IsDSDExtension(StringUtilities::GetFilePathExtension(path)))
        {
            if(DSDDecoder* dop = DSDDecoder::Open(path, true))
            {
                auto* chain   = new ChainDecoder(dop);
                auto* stretch = new TimeStretchDecoder(chain);
                VarispeedDecoder* vari = nullptr;
                auto* stream  = new MacOSMusicStream(path, new EqualiserDecoder(vari = new VarispeedDecoder(stretch)), s_Config);
                stream->m_Stretch = stretch;
                stream->m_Varispeed = vari;
                stream->m_Chain   = chain;
                stream->m_PreciseDecoders = true;
                if(stream->IsValid())
                    return SharedPtr<MusicStream>(stream);
                delete stream;
            }
        }
        if(WantsNative(s_Config))
        {
            if(AudioDecoder* precise = AudioDecoder::OpenPrecise(path))
            {
                auto* chain   = new ChainDecoder(precise);
                auto* stretch = new TimeStretchDecoder(chain);
                VarispeedDecoder* vari = nullptr;
                auto* stream  = new MacOSMusicStream(path, new EqualiserDecoder(vari = new VarispeedDecoder(stretch)), s_Config);
                stream->m_Stretch = stretch;
                stream->m_Varispeed = vari;
                stream->m_Chain   = chain;
                stream->m_PreciseDecoders = true;
                if(stream->IsValid())
                    return SharedPtr<MusicStream>(stream);
                LWARN("MusicStream: native output failed for %s, using OpenAL", path.c_str());
                delete stream;
            }
        }
#endif
#ifdef LUMOS_OPENAL
        AudioDecoder* decoder = AudioDecoder::Open(path);
        if(!decoder)
            return nullptr;
        auto* chain   = new ChainDecoder(decoder);
        auto* stretch = new TimeStretchDecoder(chain);
                VarispeedDecoder* vari = nullptr;
        auto* stream  = new ALMusicStream(path, new EqualiserDecoder(vari = new VarispeedDecoder(stretch)));
        stream->m_Stretch = stretch;
                stream->m_Varispeed = vari;
        stream->m_Chain   = chain;
        return SharedPtr<MusicStream>(stream);
#else
        return nullptr;
#endif
    }

    SharedPtr<MusicStream> MusicStream::CreateFromDecoder(const std::string& name, AudioDecoder* decoder, u32 bufferFrames, u32 bufferCount)
    {
#ifdef LUMOS_NATIVE_MUSIC
        // streams that can buffer (radio, lofi, ambient - not a live keyboard) follow the music's
        // chosen device; they never switch its rate or depth
        if(decoder && bufferFrames * bufferCount >= 8192 && WantsNative(s_Config))
        {
            MusicOutputConfig c = s_Config;
            c.matchRate         = false;
            c.matchBits         = false;
            auto* stream        = new MacOSMusicStream(name, decoder, c);
            if(stream->IsValid())
                return SharedPtr<MusicStream>(stream);
            decoder = stream->TakeDecoder();
            delete stream;
        }
#endif
#ifdef LUMOS_OPENAL
        if(!decoder)
            return nullptr;
        return SharedPtr<MusicStream>(new ALMusicStream(name, decoder, bufferFrames, bufferCount));
#else
        delete decoder;
        return nullptr;
#endif
    }

    void MusicStream::ShutdownAll()
    {
        AudioAnalyser::Shutdown();
        RemoteAudio::ShutdownAll();
#ifdef LUMOS_NATIVE_MUSIC
        MacOSMusicStream::ShutdownAll();
#endif
#ifdef LUMOS_OPENAL
        ALMusicStream::ShutdownAll();
#endif
    }

    void MusicStream::SetOutputConfig(const MusicOutputConfig& config)
    {
#ifdef LUMOS_NATIVE_MUSIC
        // hand devices back once we no longer want them switched / held
        MacOSMusicStream::RestoreDevices(!config.matchRate, !config.exclusive);
#endif
        s_Config = config;
    }
    const MusicOutputConfig& MusicStream::GetOutputConfig() { return s_Config; }

    // ---------------------------------------------------------------- scope

    MusicStream::MusicStream()
    {
        MutexInit(&m_ScopeLock);
        m_History = new float[HistorySize]();
        m_Meters  = new MeterBlockData[MeterBlocks]();
    }

    MusicStream::~MusicStream()
    {
        delete[] m_History;
        delete[] m_Meters;
        MutexDestroy(&m_ScopeLock);
    }

    void MusicStream::ScopeReset(u64 frame)
    {
        ScopedMutex lock(&m_ScopeLock);
        m_HistoryStart = m_HistoryEnd = frame;
    }

    template <typename T>
    void MusicStream::ScopePush(const T* samples, u32 frames, u32 channels, u64 first, float scale)
    {
        if(frames == 0 || channels == 0)
            return;
        ScopedMutex lock(&m_ScopeLock);
        const bool jump = first != m_HistoryEnd; // seek: the first block holds stale data
        if(jump)
            m_HistoryStart = first;
        const u32 right = channels > 1 ? 1 : 0;
        for(u32 f = 0; f < frames; f++)
        {
            u64 frame = first + f;
            float l   = samples[f * channels] * scale;
            float r   = samples[f * channels + right] * scale;
            float v   = 0.0f;
            for(u32 c = 0; c < channels; c++)
                v += samples[f * channels + c] * scale;
            m_History[frame & (HistorySize - 1)] = v / channels;

            MeterBlockData& b = m_Meters[(frame / MeterBlock) & (MeterBlocks - 1)];
            if(frame % MeterBlock == 0 || (jump && f == 0))
                b = {};
            b.peak[0] = Maths::Max(b.peak[0], fabsf(l));
            b.peak[1] = Maths::Max(b.peak[1], fabsf(r));
            b.sumSq[0] += l * l;
            b.sumSq[1] += r * r;
        }
        m_HistoryEnd = first + frames;
        if(m_HistoryEnd - m_HistoryStart > HistorySize)
            m_HistoryStart = m_HistoryEnd - HistorySize;
    }

    void MusicStream::ScopePushS16(const i16* samples, u32 frames, u32 channels, u64 first)
    {
        ScopePush(samples, frames, channels, first, 1.0f / 32768.0f);
    }

    void MusicStream::ScopePushF32(const float* samples, u32 frames, u32 channels, u64 first)
    {
        ScopePush(samples, frames, channels, first, 1.0f);
    }

    u32 MusicStream::GatherWindow(float* out, u32 count, bool centred)
    {
        if(!ScopeActive())
            return 0;
        u64 play = ScopePlayFrame();
        ScopedMutex lock(&m_ScopeLock);
        // window centred on (or ending at) the play head, clamped to what's still in the ring
        u64 before = centred ? count / 2 : count;
        u64 start  = play > before ? play - before : 0;
        if(start < m_HistoryStart)
            start = m_HistoryStart;
        if(start + count > m_HistoryEnd)
            start = m_HistoryEnd > count ? m_HistoryEnd - count : 0;
        if(start < m_HistoryStart || m_HistoryEnd - m_HistoryStart < count)
            return 0;
        for(u32 i = 0; i < count; i++)
            out[i] = m_History[(start + i) & (HistorySize - 1)];
        return count;
    }

    MusicMeters MusicStream::GetMeters()
    {
        MusicMeters m;
        if(!ScopeActive())
            return m;
        u64 play = ScopePlayFrame();
        ScopedMutex lock(&m_ScopeLock);
        u64 span   = Maths::Max<u64>(MeterBlock, (u64)m_SampleRate * 3 / 10);
        u64 end    = Maths::Min(play, m_HistoryEnd) / MeterBlock; // whole blocks before the play head
        u64 first  = (play > span ? play - span : 0) / MeterBlock;
        u64 oldest = (m_HistoryStart + MeterBlock - 1) / MeterBlock;
        u64 newest = m_HistoryEnd / MeterBlock; // blocks < this are complete
        first      = Maths::Max(first, Maths::Max(oldest, newest > MeterBlocks ? newest - MeterBlocks + 1 : 0));
        end        = Maths::Min(end, newest);
        double sum[2] = { 0, 0 };
        u32 n         = 0;
        for(u64 b = first; b < end; b++)
        {
            const MeterBlockData& d = m_Meters[b & (MeterBlocks - 1)];
            for(u32 c = 0; c < 2; c++)
            {
                m.peak[c] = Maths::Max(m.peak[c], d.peak[c]);
                sum[c] += d.sumSq[c];
            }
            n++;
        }
        if(n > 0)
            for(u32 c = 0; c < 2; c++)
                m.rms[c] = (float)sqrt(sum[c] / ((double)n * MeterBlock));
        return m;
    }

    float MusicStream::GetLevel()
    {
        float window[1024];
        if(GatherWindow(window, 1024, true) == 0)
            return 0.0f;
        float sum = 0.0f;
        for(float v : window)
            sum += v * v;
        return Maths::Min(1.0f, sqrtf(sum / 1024.0f) * 1.414f);
    }

    bool MusicStream::ComputeMagnitudes()
    {
        constexpr u32 N = 2048;
        float re[N], im[N];
        if(GatherWindow(re, N, true) == 0)
            return false;
        for(u32 i = 0; i < N; i++)
        {
            re[i] *= 0.5f - 0.5f * cosf(2.0f * Maths::M_PI * i / (N - 1)); // Hann
            im[i] = 0.0f;
        }
        FFT(re, im, N);
        for(u32 i = 0; i < N / 2; i++)
            m_Mag[i] = sqrtf(re[i] * re[i] + im[i] * im[i]) / (N * 0.25f); // ~1 for a full-scale sine
        return true;
    }

    u32 MusicStream::GetSpectrum(float* out, u32 bands)
    {
        constexpr u32 N = 2048;
        if(bands == 0)
            return 0;
        if(!ComputeMagnitudes())
        {
            for(u32 i = 0; i < bands; i++)
                out[i] = 0.0f;
            return bands;
        }

        // log-spaced bands from 40Hz to 16kHz
        const float minHz = 40.0f, maxHz = Maths::Min(16000.0f, m_SampleRate * 0.5f);
        const float binHz = (float)m_SampleRate / N;
        for(u32 b = 0; b < bands; b++)
        {
            float lo   = minHz * powf(maxHz / minHz, (float)b / bands);
            float hi   = minHz * powf(maxHz / minHz, (float)(b + 1) / bands);
            u32 i0     = Maths::Clamp((u32)(lo / binHz), 1u, N / 2 - 1);
            u32 i1     = Maths::Clamp((u32)(hi / binHz), i0 + 1, N / 2);
            float peak = 0.0f;
            for(u32 i = i0; i < i1; i++)
                peak = Maths::Max(peak, m_Mag[i]);
            float db = 20.0f * log10f(peak + 1e-9f);
            out[b]   = Maths::Clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);
        }
        return bands;
    }

    u32 MusicStream::GetFFT(float* out, u32 bins)
    {
        bins    = Maths::Min(bins, 1024u);
        bool ok = ComputeMagnitudes();
        for(u32 i = 0; i < bins; i++)
        {
            // WebAudio AnalyserNode: smoothing 0.8, dB range -100..-30
            float mag      = ok ? m_Mag[i] : 0.0f;
            m_FFTSmooth[i] = 0.8f * m_FFTSmooth[i] + 0.2f * mag;
            float db       = 20.0f * log10f(m_FFTSmooth[i] + 1e-9f);
            out[i]         = Maths::Clamp((db + 100.0f) / 70.0f, 0.0f, 1.0f);
        }
        return bins;
    }

    u32 MusicStream::GetWaveform(float* out, u32 count)
    {
        if(GatherWindow(out, count, false) == 0)
        {
            for(u32 i = 0; i < count; i++)
                out[i] = 0.0f;
        }
        return count;
    }
}
