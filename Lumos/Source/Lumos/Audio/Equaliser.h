#pragma once
#include "AudioDecoder.h"
#include "SoundChain.h"
#include <atomic>

namespace Lumos
{
    // Graphic EQ for music streams: 10 peaking bands (31 Hz - 16 kHz, octave spaced) and a preamp.
    // One setting shared by every file stream; flat = bypassed, samples untouched (bit-perfect).
    namespace Equaliser
    {
        static constexpr int Bands = 10;
        LUMOS_EXPORT const float* BandFrequencies();
        // gains in dB (-12..12 each), preamp in dB
        LUMOS_EXPORT void Set(const float* gainsDb, float preampDb);
        LUMOS_EXPORT void SetEnabled(bool on);
        LUMOS_EXPORT bool IsActive(); // enabled and not flat
    }

    class EqualiserDecoder : public AudioDecoder
    {
    public:
        explicit EqualiserDecoder(AudioDecoder* inner);
        ~EqualiserDecoder() override;

        u32 Read(i16* out, u32 frames) override;
        u32 ReadFloat(float* out, u32 frames) override;
        bool Seek(u64 frame) override;
        u64 SourceFrame(u64 outputFrame) const override { return m_Inner->SourceFrame(outputFrame); }
        std::string GetStreamTitle() const override { return m_Inner->GetStreamTitle(); }
        bool IsBuffering() const override { return m_Inner->IsBuffering(); }
        bool IsDoP() const override { return m_Inner->IsDoP(); }

    private:
        void Refresh(); // new coefficients when the settings changed
        void Process(float* data, u32 frames);
        void ProcessEQ(float* data, u32 frames);

        AudioDecoder* m_Inner;
        u32 m_Version = ~0u;
        u32 m_ChainVersion = ~0u;
        bool m_EQActive = false;
        SoundChain::Processor m_Chain;
        bool m_Active = false;
        float m_Preamp = 1.0f;
        struct Biquad
        {
            float b0, b1, b2, a1, a2;
        } m_Coef[Equaliser::Bands];
        float m_State[Equaliser::Bands][8][4] = {}; // band, channel (up to 8), x1 x2 y1 y2
        float* m_Scratch   = nullptr;
        u32 m_ScratchSize  = 0;
    };
}
