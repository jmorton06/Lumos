#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    // The music effects chain, in the order the sound goes through it: the graphic EQ (Equaliser) plus
    // crossfeed, bass, room, tape, a limiter and headphone correction. One setting shared by every music stream
    // (EqualiserDecoder runs it); nothing enabled = bypassed, samples untouched.
    namespace SoundChain
    {
        enum class StageType : u32
        {
            EQ,         // the 10-band graphic EQ (its gains live in Equaliser)
            Crossfeed,  // amount: how much of each side reaches the other ear (headphones)
            Bass,       // amount: boost, param: contour 0 = deep (30 Hz) .. 1 = warm (65 Hz)
            Room,       // amount: mix, param: size
            Tape,       // amount: drive, param: tone 0 = dark .. 1 = bright
            Correction, // the headphone correction filters below
            Limiter,    // amount: ceiling -6 dB .. -0.1 dB, param: release 20 .. 500 ms
            Count
        };

        struct Stage
        {
            StageType type = StageType::EQ;
            bool enabled   = true;
            float amount   = 0.5f; // 0..1
            float param    = 0.5f; // 0..1, meaning per type
        };

        enum class FilterType : u32
        {
            Peak,
            LowShelf,
            HighShelf
        };

        struct Filter
        {
            FilterType type = FilterType::Peak;
            float freq = 1000.0f, gainDb = 0.0f, q = 0.707f;
        };

        struct Config
        {
            TDArray<Stage> stages;     // empty = the EQ only (the default)
            TDArray<Filter> correction; // headphone correction (AutoEQ ParametricEQ style)
            float correctionPreampDb = 0.0f;
        };

        LUMOS_EXPORT void Set(const Config& config);
        LUMOS_EXPORT Config Get();
        LUMOS_EXPORT u32 Version();
        // anything besides the EQ that changes the sound
        LUMOS_EXPORT bool HasActiveEffects();
        LUMOS_EXPORT const char* StageName(StageType type);

        // Per-stream state (filters, delay lines) for the current config
        class Processor
        {
        public:
            void Prepare(const Config& config, u32 sampleRate, u32 channels);
            // runs `stage` of the config on interleaved float frames (EQ is done by the caller)
            void Run(u32 stageIndex, float* data, u32 frames);
            u32 StageCount() const { return m_Config.stages.Size(); }
            StageType TypeOf(u32 i) const { return m_Config.stages[i].type; }
            bool Enabled(u32 i) const { return m_Config.stages[i].enabled; }
            void Reset();

        private:
            struct Biquad
            {
                float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
                float z[2][4] = {};
                float Step(u32 c, float x);
            };
            Config m_Config;
            u32 m_Rate = 44100, m_Channels = 2;
            TDArray<Biquad> m_Correction;
            float m_CorrectionGain = 1.0f;
            Biquad m_BassShelf;
            float m_BassGain = 1.0f;
            // crossfeed: low-passed, slightly delayed opposite channel
            float m_XLow[2] = {};
            TDArray<float> m_XDelay; // 2 x n ring
            u32 m_XLen = 1, m_XPos = 0;
            // room: stereo delay line for early reflections
            TDArray<float> m_RoomBuf; // 2 x n ring
            u32 m_RoomLen = 1, m_RoomPos = 0;
            float m_RoomLow[2] = {};
            // tape: tone low-pass state
            float m_TapeLow[2] = {};
            // limiter: short look-ahead so the gain is down before the peak arrives
            TDArray<float> m_LimBuf; // 2 x n ring
            u32 m_LimLen = 1, m_LimPos = 0, m_LimHold = 0;
            float m_LimGain = 1.0f, m_LimTarget = 1.0f;
        };
    }
}
