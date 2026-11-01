#include "Precompiled.h"
#include "Equaliser.h"
#include <cmath>
#include <mutex>

namespace Lumos
{
    namespace
    {
        const float kFreqs[Equaliser::Bands] = { 31.25f, 62.5f, 125.f, 250.f, 500.f, 1000.f, 2000.f, 4000.f, 8000.f, 16000.f };
        std::mutex s_Lock;
        float s_Gains[Equaliser::Bands] = {};
        float s_Preamp                  = 0.0f;
        bool s_Enabled                  = true;
        std::atomic<u32> s_Version { 1 };
    }

    const float* Equaliser::BandFrequencies() { return kFreqs; }

    void Equaliser::Set(const float* gainsDb, float preampDb)
    {
        std::lock_guard<std::mutex> lock(s_Lock);
        for(int i = 0; i < Bands; i++)
            s_Gains[i] = std::fmax(-12.0f, std::fmin(12.0f, gainsDb[i]));
        s_Preamp = std::fmax(-12.0f, std::fmin(12.0f, preampDb));
        s_Version++;
    }

    void Equaliser::SetEnabled(bool on)
    {
        std::lock_guard<std::mutex> lock(s_Lock);
        s_Enabled = on;
        s_Version++;
    }

    bool Equaliser::IsActive()
    {
        std::lock_guard<std::mutex> lock(s_Lock);
        if(!s_Enabled)
            return false;
        if(std::fabs(s_Preamp) > 0.01f)
            return true;
        for(float g : s_Gains)
            if(std::fabs(g) > 0.01f)
                return true;
        return false;
    }

    EqualiserDecoder::EqualiserDecoder(AudioDecoder* inner)
        : m_Inner(inner)
    {
        m_SampleRate  = inner->GetSampleRate();
        m_Channels    = inner->GetChannels();
        m_TotalFrames = inner->GetTotalFrames();
        m_Codec       = inner->GetCodec();
        m_Lossless    = inner->IsLossless();
        m_FloatSource = inner->IsFloatSource();
        m_SourceBits  = inner->GetSourceBits();
    }

    EqualiserDecoder::~EqualiserDecoder()
    {
        delete m_Inner;
        delete[] m_Scratch;
    }

    void EqualiserDecoder::Refresh()
    {
        const u32 cv = SoundChain::Version();
        if(cv != m_ChainVersion)
        {
            m_ChainVersion = cv;
            m_Chain.Prepare(SoundChain::Get(), m_SampleRate, m_Channels);
        }
        const bool dop = m_Inner->IsDoP(); // DoP words aren't audio
        m_Active       = !dop && (m_EQActive || SoundChain::HasActiveEffects());
        const u32 v    = s_Version.load();
        if(v == m_Version)
            return;
        m_Version  = v;
        m_EQActive = Equaliser::IsActive() && !dop;
        m_Active   = !dop && (m_EQActive || SoundChain::HasActiveEffects());
        float gains[Equaliser::Bands];
        float pre;
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            for(int i = 0; i < Equaliser::Bands; i++)
                gains[i] = s_Gains[i];
            pre = s_Preamp;
        }
        m_Preamp = std::pow(10.0f, pre / 20.0f);
        // RBJ cookbook peaking filters, one octave wide
        const float Q  = 1.41f;
        const float fs = (float)(m_SampleRate ? m_SampleRate : 44100);
        for(int i = 0; i < Equaliser::Bands; i++)
        {
            const float f     = std::fmin(kFreqs[i], fs * 0.45f);
            const float A     = std::pow(10.0f, gains[i] / 40.0f);
            const float w0    = 2.0f * 3.14159265f * f / fs;
            const float alpha = std::sin(w0) / (2.0f * Q);
            const float cw    = std::cos(w0);
            const float a0    = 1.0f + alpha / A;
            m_Coef[i]         = { (1.0f + alpha * A) / a0, (-2.0f * cw) / a0, (1.0f - alpha * A) / a0,
                                  (-2.0f * cw) / a0, (1.0f - alpha / A) / a0 };
        }
    }

    // the chain in its order; no stages set = the EQ alone
    void EqualiserDecoder::Process(float* data, u32 frames)
    {
        if(m_Chain.StageCount() == 0)
        {
            if(m_EQActive)
                ProcessEQ(data, frames);
            return;
        }
        for(u32 i = 0; i < m_Chain.StageCount(); i++)
        {
            if(!m_Chain.Enabled(i))
                continue;
            if(m_Chain.TypeOf(i) == SoundChain::StageType::EQ)
            {
                if(m_EQActive)
                    ProcessEQ(data, frames);
            }
            else
                m_Chain.Run(i, data, frames);
        }
    }

    void EqualiserDecoder::ProcessEQ(float* data, u32 frames)
    {
        const u32 ch = m_Channels < 8u ? m_Channels : 8u;
        for(u32 f = 0; f < frames; f++)
        {
            for(u32 c = 0; c < ch; c++)
            {
                float x = data[f * m_Channels + c] * m_Preamp;
                for(int b = 0; b < Equaliser::Bands; b++)
                {
                    const Biquad& q = m_Coef[b];
                    float* st       = m_State[b][c];
                    const float y   = q.b0 * x + q.b1 * st[0] + q.b2 * st[1] - q.a1 * st[2] - q.a2 * st[3];
                    st[1] = st[0];
                    st[0] = x;
                    st[3] = st[2];
                    st[2] = y;
                    x     = y;
                }
                data[f * m_Channels + c] = x;
            }
        }
    }

    u32 EqualiserDecoder::ReadFloat(float* out, u32 frames)
    {
        Refresh();
        u32 n = m_Inner->ReadFloat(out, frames);
        if(m_Active && n)
            Process(out, n);
        return n;
    }

    u32 EqualiserDecoder::Read(i16* out, u32 frames)
    {
        Refresh();
        if(!m_Active)
            return m_Inner->Read(out, frames); // untouched
        const u32 samples = frames * m_Channels;
        if(m_ScratchSize < samples)
        {
            delete[] m_Scratch;
            m_Scratch     = new float[samples];
            m_ScratchSize = samples;
        }
        u32 n = m_Inner->ReadFloat(m_Scratch, frames);
        Process(m_Scratch, n);
        for(u32 i = 0; i < n * m_Channels; i++)
        {
            float v = m_Scratch[i] * 32767.0f;
            out[i]  = (i16)(v > 32767.0f ? 32767.0f : (v < -32768.0f ? -32768.0f : v));
        }
        return n;
    }

    bool EqualiserDecoder::Seek(u64 frame)
    {
        std::memset(m_State, 0, sizeof(m_State));
        m_Chain.Reset();
        return m_Inner->Seek(frame);
    }
}
