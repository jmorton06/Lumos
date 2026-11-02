#include "Precompiled.h"
#include "Varispeed.h"
#include "Maths/MathsUtilities.h"
#include <cmath>
#include <cstring>

namespace Lumos
{
    VarispeedDecoder::VarispeedDecoder(AudioDecoder* inner)
        : m_Inner(inner)
    {
        m_SampleRate  = inner->GetSampleRate();
        m_Channels    = inner->GetChannels();
        m_TotalFrames = inner->GetTotalFrames();
        m_Codec       = inner->GetCodec();
        m_Lossless    = inner->IsLossless();
        m_FloatSource = inner->IsFloatSource();
        m_SourceBits  = inner->GetSourceBits();
        PushSegment(0, 0.0, 1.0);
    }

    VarispeedDecoder::~VarispeedDecoder() { delete m_Inner; }

    void VarispeedDecoder::SetRate(float rate) { m_Target.store(Maths::Clamp(rate, 0.5f, 2.0f)); }

    void VarispeedDecoder::PushSegment(u64 outBase, double inBase, double rate)
    {
        std::lock_guard<std::mutex> lock(m_SegLock);
        // the stream plays a little behind what it decodes: keep a few old mappings
        if(m_Segments.Size() >= 8)
        {
            for(u32 i = 1; i < m_Segments.Size(); i++)
                m_Segments[i - 1] = m_Segments[i];
            m_Segments.PopBack();
        }
        m_Segments.PushBack({ outBase, inBase, rate });
    }

    u64 VarispeedDecoder::SourceFrame(u64 outputFrame) const
    {
        double inner = (double)outputFrame;
        {
            std::lock_guard<std::mutex> lock(m_SegLock);
            const Segment* seg = m_Segments.Empty() ? nullptr : &m_Segments[0];
            for(u32 i = 0; i < m_Segments.Size(); i++)
                if(m_Segments[i].outBase <= outputFrame)
                    seg = &m_Segments[i];
            if(seg)
                inner = seg->inBase + (double)((i64)outputFrame - (i64)seg->outBase) * seg->rate;
        }
        return m_Inner->SourceFrame(inner < 0.0 ? 0 : (u64)inner);
    }

    bool VarispeedDecoder::Seek(u64 frame)
    {
        bool ok    = m_Inner->Seek(frame);
        m_OutFrame = frame;
        m_Pos      = (double)frame;
        m_Buf.Clear();
        m_BufBase = frame;
        m_EOF     = false;
        {
            std::lock_guard<std::mutex> lock(m_SegLock);
            m_Segments.Clear();
        }
        m_Rate = (double)m_Target.load();
        PushSegment(frame, (double)frame, m_Rate);
        return ok;
    }

    bool VarispeedDecoder::Fill(u64 upTo)
    {
        const u32 ch = m_Channels;
        while(!m_EOF && m_BufBase + m_Buf.Size() / ch < upTo)
        {
            const u32 want = 2048;
            const size_t old = m_Buf.Size();
            m_Buf.Resize(old + (size_t)want * ch);
            u32 got = m_Inner->ReadFloat(m_Buf.Data() + old, want);
            m_Buf.Resize(old + (size_t)got * ch);
            if(got == 0)
                m_EOF = true;
        }
        return m_BufBase + m_Buf.Size() / ch >= upTo;
    }

    u32 VarispeedDecoder::ReadFloat(float* out, u32 frames)
    {
        const u32 ch = m_Channels;
        // a new rate starts a new output -> inner mapping
        const double target = (double)m_Target.load();
        if(std::fabs(target - m_Rate) > 1e-5)
        {
            m_Rate = target;
            if(m_Rate == 1.0)
                m_Pos = std::floor(m_Pos + 0.5); // back to whole frames: under a frame's jump, and the next reads are untouched
            PushSegment(m_OutFrame, m_Pos, m_Rate);
        }
        // at 1x on a whole frame: what's buffered as it is, then straight through, untouched
        if(m_Rate == 1.0 && m_Pos == std::floor(m_Pos))
        {
            u32 done = 0;
            const u64 idx  = (u64)m_Pos - m_BufBase;
            const u64 have = m_Buf.Size() / ch;
            if(idx < have)
            {
                done = (u32)Maths::Min<u64>(frames, have - idx);
                memcpy(out, m_Buf.Data() + idx * ch, (size_t)done * ch * sizeof(float));
                m_Pos += done;
                m_OutFrame += done;
                if(done == frames)
                    return done;
            }
            m_Buf.Clear();
            u32 n = m_Inner->ReadFloat(out + (size_t)done * ch, frames - done);
            m_OutFrame += n;
            m_Pos += n;
            m_BufBase = (u64)m_Pos;
            return done + n;
        }
        u32 done = 0;
        for(; done < frames; done++)
        {
            const double p = m_Pos;
            const i64 i    = (i64)std::floor(p);
            const float t  = (float)(p - (double)i);
            // 4 points around the read position (i-1 .. i+2)
            if(!Fill((u64)(i + 3)) && (u64)(i + 1) >= m_BufBase + m_Buf.Size() / ch)
                break; // ran out
            for(u32 c = 0; c < ch; c++)
            {
                auto at = [&](i64 k) -> float {
                    if(k < (i64)m_BufBase)
                        k = (i64)m_BufBase;
                    const i64 last = (i64)(m_BufBase + m_Buf.Size() / ch) - 1;
                    if(k > last)
                        k = last;
                    return m_Buf[(size_t)(k - (i64)m_BufBase) * ch + c];
                };
                const float y0 = at(i - 1), y1 = at(i), y2 = at(i + 1), y3 = at(i + 2);
                // Catmull-Rom
                const float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
                const float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                const float cc = -0.5f * y0 + 0.5f * y2;
                out[done * ch + c] = ((a * t + b) * t + cc) * t + y1;
            }
            m_Pos += m_Rate;
            m_OutFrame++;
        }
        // drop what's behind the read position (keep one frame for the interpolation)
        const i64 keepFrom = (i64)std::floor(m_Pos) - 1;
        if(keepFrom > (i64)m_BufBase + 4096)
        {
            const size_t drop = (size_t)(keepFrom - (i64)m_BufBase) * ch;
            memmove(m_Buf.Data(), m_Buf.Data() + drop, (m_Buf.Size() - drop) * sizeof(float));
            m_Buf.Resize(m_Buf.Size() - drop);
            m_BufBase = (u64)keepFrom;
        }
        return done;
    }

    u32 VarispeedDecoder::Read(i16* out, u32 frames)
    {
        m_Tmp.Resize((size_t)frames * m_Channels);
        u32 n = ReadFloat(m_Tmp.Data(), frames);
        for(u32 i = 0; i < n * m_Channels; i++)
            out[i] = (i16)Maths::Clamp(m_Tmp[i] * 32767.0f, -32768.0f, 32767.0f);
        return n;
    }
}
