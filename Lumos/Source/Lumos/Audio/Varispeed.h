#pragma once
#include "AudioDecoder.h"
#include "Core/DataStructures/TDArray.h"
#include <atomic>
#include <mutex>

namespace Lumos
{
    // Tape-style speed: plays faster / slower and higher / lower together (a resampler, cubic), unlike
    // TimeStretchDecoder which keeps the pitch. At 1x the samples pass straight through.
    // Seek takes source frames; SourceFrame maps a stream's output frame back through the inner decoder.
    class LUMOS_EXPORT VarispeedDecoder : public AudioDecoder
    {
    public:
        explicit VarispeedDecoder(AudioDecoder* inner);
        ~VarispeedDecoder() override;

        u32 Read(i16* out, u32 frames) override;
        u32 ReadFloat(float* out, u32 frames) override;
        bool Seek(u64 frame) override;
        u64 SourceFrame(u64 outputFrame) const override;
        std::string GetStreamTitle() const override { return m_Inner->GetStreamTitle(); }
        bool IsBuffering() const override { return m_Inner->IsBuffering(); }
        bool IsDoP() const override { return m_Inner->IsDoP(); }

        // 0.5 - 2 (1 = off); takes effect at the next chunk
        void SetRate(float rate);
        float GetRate() const { return m_Target.load(); }

    private:
        struct Segment
        {
            u64 outBase;
            double inBase;
            double rate;
        };
        void PushSegment(u64 outBase, double inBase, double rate);
        bool Fill(u64 upTo); // inner frames buffered through upTo (exclusive)

        AudioDecoder* m_Inner;
        std::atomic<float> m_Target { 1.0f };
        double m_Rate = 1.0;
        mutable std::mutex m_SegLock;
        TDArray<Segment> m_Segments;
        u64 m_OutFrame = 0;  // next output frame
        double m_Pos   = 0;  // inner frame the next output frame reads at
        TDArray<float> m_Buf; // inner frames from m_BufBase
        u64 m_BufBase = 0;
        bool m_EOF    = false;
        TDArray<float> m_Tmp;
    };
}
