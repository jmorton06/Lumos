#pragma once
#include "Audio/AudioDecoder.h"
#include "Core/DataStructures/TDArray.h"
#include <atomic>
#include <mutex>

namespace Lumos
{
    // Wraps a decoder to play it faster or slower without changing pitch (WSOLA: overlapping windowed
    // frames, each spliced where it best matches the last). At 1x it passes the source through untouched.
    // Seek takes source frames; SourceFrame maps a stream's output frame back to the source.
    class LUMOS_EXPORT TimeStretchDecoder : public AudioDecoder
    {
    public:
        explicit TimeStretchDecoder(AudioDecoder* inner);
        ~TimeStretchDecoder() override;

        u32 Read(i16* out, u32 frames) override;
        u32 ReadFloat(float* out, u32 frames) override;
        bool Seek(u64 frame) override;
        u64 SourceFrame(u64 outputFrame) const override;
        std::string GetStreamTitle() const override { return m_Inner->GetStreamTitle(); }
        bool IsBuffering() const override { return m_Inner->IsBuffering(); }
        bool IsDoP() const override { return m_Inner->IsDoP(); }

        // 0.5 - 3; takes effect at the next chunk the stream decodes
        void SetSpeed(float speed);
        float GetSpeed() const { return m_Target.load(); }

    private:
        struct Segment
        {
            u64 outBase;
            double inBase;
            double speed;
        };

        void ApplySpeed();
        void ResetStretch(double sourceFrame);
        bool FillTo(u64 sourceEnd);
        float Mono(u64 sourceFrame) const;
        u64 BestSplice(u64 nominal) const;
        void ProduceHop();
        void PushSegment(u64 outBase, double inBase, double speed);

        AudioDecoder* m_Inner;
        std::atomic<float> m_Target { 1.0f };
        double m_Speed = 1.0;

        mutable std::mutex m_SegLock;
        TDArray<Segment> m_Segments;
        u64 m_OutFrame = 0; // number of the next output frame

        // stretching state
        u32 m_N = 0, m_Ho = 0, m_Delta = 0;
        TDArray<float> m_Window;
        TDArray<float> m_In;     // interleaved source from m_InBase
        u64 m_InBase    = 0;
        bool m_InEOF    = false;
        double m_Nominal = 0.0;  // source frame the next frame should start near
        i64 m_PrevNat    = -1;   // where the previous frame's natural continuation begins
        TDArray<float> m_Tail;   // second half of the previous windowed frame
        TDArray<float> m_Out;    // produced but not yet handed out
        u32 m_OutHead   = 0;
        bool m_Finished = false;
        TDArray<float> m_Tmp;
    };
}
