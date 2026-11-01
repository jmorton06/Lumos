#include "Precompiled.h"
#include "TimeStretch.h"
#include "Maths/MathsUtilities.h"

#include <cmath>
#include <cstring>

namespace Lumos
{
    TimeStretchDecoder::TimeStretchDecoder(AudioDecoder* inner)
        : m_Inner(inner)
    {
        m_SampleRate  = inner->GetSampleRate();
        m_Channels    = inner->GetChannels();
        m_TotalFrames = inner->GetTotalFrames();
        m_Codec       = inner->GetCodec();
        m_Lossless    = inner->IsLossless();
        m_FloatSource = inner->IsFloatSource();
        m_SourceBits  = inner->GetSourceBits();

        // 40ms Hann frames at 50% overlap, splices searched +-12ms
        m_N = Maths::Max(256u, (u32)(m_SampleRate * 0.04f)) & ~1u;
        m_Ho    = m_N / 2;
        m_Delta = (u32)(m_SampleRate * 0.012f);
        m_Window.Resize(m_N);
        for(u32 i = 0; i < m_N; i++)
            m_Window[i] = 0.5f - 0.5f * cosf(2.0f * Maths::M_PI * (float)i / (float)m_N);
        PushSegment(0, 0.0, 1.0);
    }

    TimeStretchDecoder::~TimeStretchDecoder()
    {
        delete m_Inner;
    }

    void TimeStretchDecoder::SetSpeed(float speed)
    {
        m_Target.store(Maths::Clamp(speed, 0.5f, 3.0f));
    }

    void TimeStretchDecoder::PushSegment(u64 outBase, double inBase, double speed)
    {
        std::lock_guard<std::mutex> lock(m_SegLock);
        // the stream plays a little behind what it decodes: keep a few old mappings
        if(m_Segments.Size() >= 8)
        {
            for(u32 i = 1; i < m_Segments.Size(); i++)
                m_Segments[i - 1] = m_Segments[i];
            m_Segments.PopBack();
        }
        m_Segments.PushBack({ outBase, inBase, speed });
    }

    u64 TimeStretchDecoder::SourceFrame(u64 outputFrame) const
    {
        std::lock_guard<std::mutex> lock(m_SegLock);
        const Segment* seg = m_Segments.Empty() ? nullptr : &m_Segments[0];
        for(u32 i = 0; i < m_Segments.Size(); i++)
            if(m_Segments[i].outBase <= outputFrame)
                seg = &m_Segments[i];
        if(!seg)
            return outputFrame;
        double src = seg->inBase + (double)((i64)outputFrame - (i64)seg->outBase) * seg->speed;
        return src < 0.0 ? 0 : (u64)src;
    }

    void TimeStretchDecoder::ResetStretch(double sourceFrame)
    {
        m_In.Clear();
        m_InBase   = (u64)sourceFrame;
        m_InEOF    = false;
        m_Nominal  = sourceFrame;
        m_PrevNat  = -1;
        m_Tail.Clear();
        m_Tail.Resize((size_t)m_Ho * m_Channels, 0.0f);
        m_Out.Clear();
        m_OutHead  = 0;
        m_Finished = false;
    }

    // Called between chunks: a speed change starts a new output -> source mapping
    void TimeStretchDecoder::ApplySpeed()
    {
        double target = m_Inner->IsDoP() ? 1.0 : (double)m_Target.load(); // DoP can't be stretched
        if(std::fabs(target - m_Speed) < 1e-4)
            return;
        double src;
        if(m_Speed == 1.0)
            src = (double)SourceFrame(m_OutFrame); // passthrough: exact
        else
            src = m_Nominal;                       // stretching: where the next frame would start
        bool wasStretching = m_Speed != 1.0;
        m_Speed            = target;
        if(m_Speed == 1.0)
        {
            // back to passthrough: the source was read ahead into the buffer, so rewind to where we are
            if(wasStretching)
                m_Inner->Seek((u64)src);
        }
        else if(!wasStretching)
            ResetStretch(src);
        PushSegment(m_OutFrame, src, m_Speed);
    }

    bool TimeStretchDecoder::Seek(u64 frame)
    {
        bool ok = m_Inner->Seek(frame);
        m_OutFrame = frame;
        {
            std::lock_guard<std::mutex> lock(m_SegLock);
            m_Segments.Clear();
        }
        m_Speed = (double)m_Target.load();
        PushSegment(frame, (double)frame, m_Speed);
        if(m_Speed != 1.0)
            ResetStretch((double)frame);
        return ok;
    }

    bool TimeStretchDecoder::FillTo(u64 sourceEnd)
    {
        const u32 ch = m_Channels;
        while(!m_InEOF && m_InBase + m_In.Size() / ch < sourceEnd)
        {
            u64 have = m_InBase + m_In.Size() / ch;
            u32 want = (u32)Maths::Min<u64>(sourceEnd - have + 1024, 16384);
            size_t old = m_In.Size();
            m_In.Resize(old + (size_t)want * ch);
            u32 got = m_Inner->ReadFloat(m_In.Data() + old, want);
            m_In.Resize(old + (size_t)got * ch);
            if(got == 0)
                m_InEOF = true;
        }
        return m_InBase + m_In.Size() / ch >= sourceEnd;
    }

    float TimeStretchDecoder::Mono(u64 f) const
    {
        const u32 ch = m_Channels;
        if(f < m_InBase)
            return 0.0f;
        size_t i = (size_t)(f - m_InBase) * ch;
        if(i + ch > m_In.Size())
            return 0.0f;
        float s = 0.0f;
        for(u32 c = 0; c < ch; c++)
            s += m_In[i + c];
        return s;
    }

    // The start near `nominal` whose first half best matches the previous frame's continuation
    u64 TimeStretchDecoder::BestSplice(u64 nominal) const
    {
        if(m_PrevNat < 0)
            return nominal;
        const u64 lo = nominal > m_Delta ? Maths::Max<u64>(nominal - m_Delta, m_InBase) : m_InBase;
        const u64 hi = nominal + m_Delta;
        auto score = [&](u64 cand)
        {
            float xy = 0.0f, yy = 1e-9f;
            for(u32 k = 0; k < m_Ho; k += 4)
            {
                float x = Mono((u64)m_PrevNat + k), y = Mono(cand + k);
                xy += x * y;
                yy += y * y;
            }
            return xy / std::sqrt(yy);
        };
        u64 best = nominal;
        float bestScore = -1e30f;
        for(u64 c = lo; c <= hi; c += 4)
        {
            float sc = score(c);
            if(sc > bestScore) { bestScore = sc; best = c; }
        }
        for(u64 c = best > lo + 3 ? best - 3 : lo; c <= best + 3 && c <= hi; c++)
        {
            float sc = score(c);
            if(sc > bestScore) { bestScore = sc; best = c; }
        }
        return best;
    }

    void TimeStretchDecoder::ProduceHop()
    {
        const u32 ch = m_Channels;
        u64 nominal  = (u64)m_Nominal;
        bool haveAll = FillTo(nominal + m_Delta + m_N);
        u64 avail    = m_InBase + m_In.Size() / ch;
        if(!haveAll && nominal >= avail)
        {
            // source finished: let the last frame's tail ring out once
            size_t base = m_Out.Size();
            m_Out.Resize(base + m_Tail.Size());
            memcpy(m_Out.Data() + base, m_Tail.Data(), m_Tail.Size() * sizeof(float));
            m_Finished = true;
            return;
        }

        u64 seg = BestSplice(nominal);
        size_t base = m_Out.Size();
        m_Out.Resize(base + (size_t)m_Ho * ch);
        float* out = m_Out.Data() + base;
        for(u32 i = 0; i < m_N; i++)
        {
            u64 f     = seg + i;
            size_t ii = (size_t)(f - m_InBase) * ch;
            bool in   = f >= m_InBase && ii + ch <= m_In.Size();
            for(u32 c = 0; c < ch; c++)
            {
                float v = (in ? m_In[ii + c] : 0.0f) * m_Window[i];
                if(i < m_Ho)
                    out[(size_t)i * ch + c] = m_Tail[(size_t)i * ch + c] + v;
                else
                    m_Tail[(size_t)(i - m_Ho) * ch + c] = v;
            }
        }
        m_PrevNat = (i64)(seg + m_Ho);
        m_Nominal += m_Ho * m_Speed;

        // drop source nobody will look at again
        u64 keepFrom = Maths::Min<u64>((u64)m_PrevNat, (u64)m_Nominal);
        keepFrom     = keepFrom > m_Delta + 8 ? keepFrom - m_Delta - 8 : 0;
        if(keepFrom > m_InBase)
        {
            size_t drop = (size_t)Maths::Min<u64>(keepFrom - m_InBase, m_In.Size() / ch) * ch;
            if(drop > 0)
            {
                memmove(m_In.Data(), m_In.Data() + drop, (m_In.Size() - drop) * sizeof(float));
                m_In.Resize(m_In.Size() - drop);
                m_InBase += drop / ch;
            }
        }
    }

    u32 TimeStretchDecoder::ReadFloat(float* out, u32 frames)
    {
        if(m_OutHead >= m_Out.Size())
            ApplySpeed();
        if(m_Speed == 1.0 && m_OutHead >= m_Out.Size())
        {
            u32 n = m_Inner->ReadFloat(out, frames);
            m_OutFrame += n;
            return n;
        }

        const u32 ch = m_Channels;
        u32 written  = 0;
        while(written < frames)
        {
            if(m_OutHead >= m_Out.Size())
            {
                m_Out.Clear();
                m_OutHead = 0;
                if(m_Finished)
                    break;
                // a new speed applies at the next frame boundary
                ApplySpeed();
                if(m_Speed == 1.0)
                {
                    m_OutFrame += written;
                    u32 n = m_Inner->ReadFloat(out + (size_t)written * ch, frames - written);
                    m_OutFrame += n;
                    return written + n;
                }
                ProduceHop();
                if(m_Out.Empty())
                    break;
            }
            u32 take = Maths::Min<u32>(frames - written, (u32)((m_Out.Size() - m_OutHead) / ch));
            memcpy(out + (size_t)written * ch, m_Out.Data() + m_OutHead, (size_t)take * ch * sizeof(float));
            m_OutHead += take * ch;
            written += take;
        }
        m_OutFrame += written;
        return written;
    }

    u32 TimeStretchDecoder::Read(i16* out, u32 frames)
    {
        if(m_OutHead >= m_Out.Size())
            ApplySpeed();
        if(m_Speed == 1.0 && m_OutHead >= m_Out.Size())
        {
            u32 n = m_Inner->Read(out, frames);
            m_OutFrame += n;
            return n;
        }
        m_Tmp.Resize((size_t)frames * m_Channels);
        u32 n = ReadFloat(m_Tmp.Data(), frames);
        for(size_t i = 0; i < (size_t)n * m_Channels; i++)
            out[i] = (i16)Maths::Clamp(m_Tmp[i] * 32767.0f, -32768.0f, 32767.0f);
        return n;
    }
}
