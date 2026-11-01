#include "Precompiled.h"
#include "SoundChain.h"
#include <atomic>
#include <cmath>
#include <mutex>

namespace Lumos
{
    namespace SoundChain
    {
        namespace
        {
            std::mutex s_Lock;
            Config s_Config;
            std::atomic<u32> s_Version { 1 };
            const float kPi = 3.14159265f;

            // RBJ cookbook
            void Design(FilterType type, float fs, float f, float gainDb, float q, float& b0, float& b1, float& b2, float& a1, float& a2)
            {
                f               = std::fmin(f, fs * 0.45f);
                const float A   = std::pow(10.0f, gainDb / 40.0f);
                const float w0  = 2.0f * kPi * f / fs;
                const float cw  = std::cos(w0);
                const float al  = std::sin(w0) / (2.0f * std::fmax(0.1f, q));
                const float sA  = std::sqrt(A);
                float a0 = 1;
                switch(type)
                {
                case FilterType::Peak:
                    b0 = 1 + al * A; b1 = -2 * cw; b2 = 1 - al * A;
                    a0 = 1 + al / A; a1 = -2 * cw; a2 = 1 - al / A;
                    break;
                case FilterType::LowShelf:
                    b0 = A * ((A + 1) - (A - 1) * cw + 2 * sA * al);
                    b1 = 2 * A * ((A - 1) - (A + 1) * cw);
                    b2 = A * ((A + 1) - (A - 1) * cw - 2 * sA * al);
                    a0 = (A + 1) + (A - 1) * cw + 2 * sA * al;
                    a1 = -2 * ((A - 1) + (A + 1) * cw);
                    a2 = (A + 1) + (A - 1) * cw - 2 * sA * al;
                    break;
                case FilterType::HighShelf:
                    b0 = A * ((A + 1) + (A - 1) * cw + 2 * sA * al);
                    b1 = -2 * A * ((A - 1) + (A + 1) * cw);
                    b2 = A * ((A + 1) + (A - 1) * cw - 2 * sA * al);
                    a0 = (A + 1) - (A - 1) * cw + 2 * sA * al;
                    a1 = 2 * ((A - 1) - (A + 1) * cw);
                    a2 = (A + 1) - (A - 1) * cw - 2 * sA * al;
                    break;
                }
                b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
            }

            float OnePole(float fs, float fc) { return 1.0f - std::exp(-2.0f * kPi * fc / fs); }
        }

        const char* StageName(StageType type)
        {
            switch(type)
            {
            case StageType::EQ: return "eq";
            case StageType::Crossfeed: return "crossfeed";
            case StageType::Bass: return "bass";
            case StageType::Room: return "room";
            case StageType::Tape: return "tape";
            case StageType::Correction: return "correction";
            case StageType::Limiter: return "limiter";
            default: return "?";
            }
        }

        void Set(const Config& config)
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            s_Config = config;
            s_Version++;
        }

        Config Get()
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            return s_Config;
        }

        u32 Version() { return s_Version.load(); }

        bool HasActiveEffects()
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            for(auto& s : s_Config.stages)
            {
                if(!s.enabled || s.type == StageType::EQ)
                    continue;
                if(s.type == StageType::Correction && s_Config.correction.Empty())
                    continue;
                if(s.amount <= 0.001f && s.type != StageType::Correction && s.type != StageType::Limiter)
                    continue;
                return true;
            }
            return false;
        }

        float Processor::Biquad::Step(u32 c, float x)
        {
            float* s      = z[c];
            const float y = b0 * x + b1 * s[0] + b2 * s[1] - a1 * s[2] - a2 * s[3];
            s[1] = s[0];
            s[0] = x;
            s[3] = s[2];
            s[2] = y;
            return y;
        }

        void Processor::Prepare(const Config& config, u32 sampleRate, u32 channels)
        {
            m_Config   = config;
            m_Rate     = sampleRate ? sampleRate : 44100;
            m_Channels = channels ? (channels > 2 ? 2 : channels) : 2;
            const float fs = (float)m_Rate;

            m_Correction.Clear();
            for(auto& f : config.correction)
            {
                Biquad b;
                Design(f.type, fs, f.freq, f.gainDb, f.q, b.b0, b.b1, b.b2, b.a1, b.a2);
                m_Correction.PushBack(b);
            }
            m_CorrectionGain = std::pow(10.0f, config.correctionPreampDb / 20.0f);

            for(auto& s : config.stages)
                if(s.type == StageType::Bass)
                {
                    // shelf corner a little above the contour frequency; half the boost taken back
                    // as headroom so it doesn't clip
                    const float f  = 30.0f + s.param * 35.0f;
                    const float db = s.amount * 9.0f;
                    Design(FilterType::LowShelf, fs, f * 2.0f, db, 0.7f, m_BassShelf.b0, m_BassShelf.b1, m_BassShelf.b2, m_BassShelf.a1, m_BassShelf.a2);
                    m_BassGain = std::pow(10.0f, -db * 0.5f / 20.0f);
                }

            m_XLen = (u32)std::fmax(1.0f, 0.00028f * fs); // ~0.3 ms between the ears
            m_XDelay.Resize(2 * m_XLen, 0.0f);
            m_RoomLen = (u32)(0.09f * fs) + 1;
            m_RoomBuf.Resize(2 * m_RoomLen, 0.0f);
            m_LimLen = (u32)std::fmax(1.0f, 0.0015f * fs);
            m_LimBuf.Resize(2 * m_LimLen, 0.0f);
            Reset();
        }

        void Processor::Reset()
        {
            for(auto& b : m_Correction)
                memset(b.z, 0, sizeof(b.z));
            memset(m_BassShelf.z, 0, sizeof(m_BassShelf.z));
            for(auto& v : m_XDelay)
                v = 0.0f;
            for(auto& v : m_RoomBuf)
                v = 0.0f;
            for(auto& v : m_LimBuf)
                v = 0.0f;
            m_LimPos = m_LimHold = 0;
            m_LimGain = m_LimTarget = 1.0f;
            m_XLow[0] = m_XLow[1] = m_RoomLow[0] = m_RoomLow[1] = m_TapeLow[0] = m_TapeLow[1] = 0.0f;
            m_XPos = m_RoomPos = 0;
        }

        void Processor::Run(u32 i, float* data, u32 frames)
        {
            const Stage& s = m_Config.stages[i];
            const u32 ch   = m_Channels;
            const float fs = (float)m_Rate;
            switch(s.type)
            {
            case StageType::Correction:
                for(u32 f = 0; f < frames; f++)
                    for(u32 c = 0; c < ch; c++)
                    {
                        float x = data[f * ch + c] * m_CorrectionGain;
                        for(auto& b : m_Correction)
                            x = b.Step(c, x);
                        data[f * ch + c] = x;
                    }
                break;

            case StageType::Bass:
                for(u32 f = 0; f < frames; f++)
                    for(u32 c = 0; c < ch; c++)
                        data[f * ch + c] = m_BassShelf.Step(c, data[f * ch + c] * m_BassGain);
                break;

            case StageType::Crossfeed:
            {
                if(ch < 2)
                    break;
                // each ear gets a low-passed, slightly late share of the other side (Bauer-style)
                const float level = s.amount * 0.55f;
                const float k     = OnePole(fs, 700.0f);
                const float norm  = 1.0f / (1.0f + level * 0.6f);
                for(u32 f = 0; f < frames; f++)
                {
                    float* p = data + f * 2;
                    const float l = p[0], r = p[1];
                    m_XLow[0] += k * (l - m_XLow[0]);
                    m_XLow[1] += k * (r - m_XLow[1]);
                    float* d  = m_XDelay.Data();
                    const float dl = d[m_XPos], dr = d[m_XLen + m_XPos];
                    d[m_XPos]          = m_XLow[0];
                    d[m_XLen + m_XPos] = m_XLow[1];
                    m_XPos             = (m_XPos + 1) % m_XLen;
                    p[0]               = (l + level * dr) * norm;
                    p[1]               = (r + level * dl) * norm;
                }
                break;
            }

            case StageType::Room:
            {
                // early reflections: a handful of taps off the walls, darker than the direct sound,
                // alternating sides
                static const float kTapMs[6] = { 7.0f, 11.0f, 17.0f, 23.0f, 31.0f, 41.0f };
                static const float kGain[6]  = { 0.50f, 0.42f, 0.35f, 0.30f, 0.24f, 0.20f };
                const float scale = 0.6f + s.param * 0.9f;
                u32 taps[6];
                for(int t = 0; t < 6; t++)
                    taps[t] = (u32)std::fmin((float)(m_RoomLen - 1), kTapMs[t] * scale * 0.001f * fs);
                const float mix = s.amount * 0.6f;
                const float dry = 1.0f - s.amount * 0.25f;
                const float k   = OnePole(fs, 5000.0f);
                float* b        = m_RoomBuf.Data();
                for(u32 f = 0; f < frames; f++)
                {
                    float in[2] = { data[f * ch], data[f * ch + (ch > 1 ? 1 : 0)] };
                    for(u32 c = 0; c < 2; c++)
                    {
                        m_RoomLow[c] += k * (in[c] - m_RoomLow[c]);
                        b[c * m_RoomLen + m_RoomPos] = m_RoomLow[c];
                    }
                    float wet[2] = { 0, 0 };
                    for(int t = 0; t < 6; t++)
                    {
                        const u32 idx = (m_RoomPos + m_RoomLen - taps[t]) % m_RoomLen;
                        const u32 src = (t & 1) ? 1 : 0; // odd taps come from the far side
                        wet[0] += kGain[t] * b[(src ^ 0) * m_RoomLen + idx];
                        wet[1] += kGain[t] * b[(src ^ 1) * m_RoomLen + idx];
                    }
                    m_RoomPos = (m_RoomPos + 1) % m_RoomLen;
                    for(u32 c = 0; c < ch; c++)
                        data[f * ch + c] = in[c] * dry + wet[c] * mix;
                }
                break;
            }

            case StageType::Tape:
            {
                // soft saturation (tanh, unity gain for quiet signals, peaks rounded off) blended in,
                // then a tone roll-off
                const float drive = 1.0f + s.amount * 1.5f;
                const float blend = s.amount * 0.5f;
                const float k     = OnePole(fs, 7000.0f + s.param * 13000.0f);
                for(u32 f = 0; f < frames; f++)
                    for(u32 c = 0; c < ch; c++)
                    {
                        float x   = data[f * ch + c];
                        float sat = std::tanh(x * drive) / drive;
                        float y   = x + (sat - x) * blend;
                        m_TapeLow[c] += k * (y - m_TapeLow[c]);
                        data[f * ch + c] = m_TapeLow[c];
                    }
                break;
            }

            case StageType::Limiter:
            {
                // channels share one gain so the image doesn't shift. The gain a peak needs is held
                // for the look-ahead and reached within it, then let go at the release rate.
                const float ceiling = std::pow(10.0f, (-6.0f + s.amount * 5.9f) / 20.0f);
                const float attack  = OnePole(fs, 3.0f * fs / (2.0f * kPi * (float)m_LimLen));
                const float release = 1.0f - std::exp(-1.0f / ((0.02f + s.param * 0.48f) * fs));
                float* b            = m_LimBuf.Data();
                for(u32 f = 0; f < frames; f++)
                {
                    float in[2] = { data[f * ch], data[f * ch + (ch > 1 ? 1 : 0)] };
                    const float peak = std::fmax(std::fabs(in[0]), std::fabs(in[1]));
                    const float need = peak > ceiling ? ceiling / peak : 1.0f;
                    if(need <= m_LimTarget)
                    {
                        m_LimTarget = need;
                        m_LimHold   = m_LimLen;
                    }
                    else if(m_LimHold > 0)
                        m_LimHold--;
                    else
                        m_LimTarget = need;
                    m_LimGain += (m_LimTarget - m_LimGain) * (m_LimTarget < m_LimGain ? attack : release);

                    const float out[2] = { b[m_LimPos], b[m_LimLen + m_LimPos] };
                    b[m_LimPos]            = in[0];
                    b[m_LimLen + m_LimPos] = in[1];
                    m_LimPos               = (m_LimPos + 1) % m_LimLen;
                    for(u32 c = 0; c < ch; c++)
                    {
                        // whatever the envelope didn't quite catch stops at the ceiling
                        const float y    = out[c] * m_LimGain;
                        data[f * ch + c] = y > ceiling ? ceiling : (y < -ceiling ? -ceiling : y);
                    }
                }
                break;
            }

            default:
                break;
            }
        }
    }
}
