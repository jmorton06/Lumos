#pragma once
#include "Core/Core.h"
#include "Core/Mutex.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    class AudioDecoder;

    // Small subtractive synth / sampler with buses and effects, rendered on the audio thread.
    // Callers schedule notes and parameter changes in seconds on the synth's own clock (Now());
    // anything scheduled in the past plays at the next control block (~1.5ms). Render either
    // live through MusicStream (CreateDecoder) or offline with Render() + SaveWav().
    class LUMOS_EXPORT SynthEngine
    {
    public:
        enum Wave : u8
        {
            Wave_Sine,
            Wave_Saw,
            Wave_Square,
            Wave_Triangle,
            Wave_Noise, // white
            Wave_Pink,
            Wave_Brown,
            Wave_Dust, // random clicks, DustDensity per second (rain, vinyl crackle)
            Wave_Count
        };

        enum FilterMode : u8
        {
            Filter_Off,
            Filter_LowPass,
            Filter_HighPass,
            Filter_BandPass,
            Filter_Count
        };

        enum LfoTarget : u8
        {
            Lfo_Off,
            Lfo_Pitch,  // depth in semitones
            Lfo_Cutoff, // depth in octaves
            Lfo_Amp,    // depth 0..1
            Lfo_Pan,    // depth 0..1
            Lfo_Count
        };

        // Instrument parameters. Waves / modes / targets are stored as their enum value.
        enum Param : u8
        {
            P_Osc1Wave, P_Osc1Level, P_Osc1Octave,
            P_Osc2Wave, P_Osc2Level, P_Osc2Octave, P_Osc2Detune, // detune in semitones
            P_PulseWidth, P_DustDensity,
            P_PitchEnv, P_PitchDecay, // pitch drop in semitones at note start, seconds to settle (kicks, toms)
            P_Attack, P_Decay, P_Sustain, P_Release,
            P_FilterMode, P_Cutoff, P_Resonance, P_FilterEnv, P_KeyTrack, // FilterEnv in octaves
            P_FilterAttack, P_FilterDecay, P_FilterSustain, P_FilterRelease,
            P_LfoTarget, P_LfoWave, P_LfoRate, P_LfoDepth,
            P_Glide, P_Volume, P_Pan, P_Voices, P_Bus, P_VelocitySens, P_Tune, // Tune in semitones
            P_Sample, P_SampleRoot, P_SampleLoop, P_SampleStart, P_SampleEnd, // start / end 0..1
            P_Count
        };

        enum EffectType : u8
        {
            FX_Filter,     // mode, cutoff, resonance
            FX_Bitcrush,   // bits, downsample, mix
            FX_Saturate,   // drive, mix
            FX_Delay,      // time, feedback, mix, pingpong, damp
            FX_Reverb,     // size, damp, mix, width
            FX_Tape,       // wow, flutter, drive, age (high cut)
            FX_Compressor, // threshold dB, ratio, attack, release, makeup dB, sidechain bus (-1 = self)
            FX_Gain,       // gain dB
            FX_Count
        };
        static constexpr u32 MaxEffectParams = 6;

        static constexpr u32 MaxVoices = 32;
        static constexpr u32 BlockFrames = 64; // control rate: params, envelopes targets and events apply per block

        explicit SynthEngine(u32 sampleRate = 44100);
        ~SynthEngine();

        u32 GetSampleRate() const { return m_SampleRate; }
        double Now() const; // seconds rendered so far

        // ---- instruments (id >= 0) ----
        i32 AddInstrument();
        void RemoveInstrument(i32 inst);
        void SetParam(i32 inst, Param p, float value);
        // Ramps a parameter from its value at `time` to `value` over `rampSeconds`
        void ScheduleParam(i32 inst, Param p, float value, double time, float rampSeconds);
        float GetParam(i32 inst, Param p) const;

        // ---- notes ----
        // duration <= 0: held until NoteOff
        void NoteOn(i32 inst, float note, float velocity, double time, float duration = 0.0f);
        void NoteOff(i32 inst, float note, double time);
        void AllNotesOff(i32 inst = -1); // -1: every instrument, immediately
        void ClearScheduled(i32 inst = -1);

        // ---- buses (0 = master) ----
        i32 AddBus();
        void SetBusVolume(i32 bus, float volume);
        void SetBusPan(i32 bus, float pan);
        void SetBusOutput(i32 bus, i32 output);
        void ScheduleBusVolume(i32 bus, float volume, double time, float rampSeconds);
        float GetBusLevel(i32 bus) const; // recent peak, for meters
        i32 AddEffect(i32 bus, EffectType type);
        void SetEffectParam(i32 bus, i32 effect, u32 param, float value);
        void SetEffectEnabled(i32 bus, i32 effect, bool enabled);
        void ClearEffects(i32 bus);

        // ---- samples ----
        // Decodes a whole file (mp3/ogg/wav/...) into memory, resampled to the engine rate. -1 on failure.
        i32 LoadSample(const std::string& path, float maxSeconds = 30.0f);
        i32 AddSample(const float* interleaved, u32 frames, u32 channels, u32 sampleRate);
        float GetSampleLength(i32 sample) const;

        // ---- rendering ----
        void RenderFloat(float* stereo, u32 frames);
        // Offline: renders into the capture buffer (the engine must not be streaming at the same time)
        void Render(double seconds);
        bool SaveWav(const std::string& path, bool clearCapture = true);
        double GetCaptureLength() const;
        void ClearCapture();

        // A decoder that pulls from the engine forever (stereo s16); it keeps the engine alive.
        static AudioDecoder* CreateDecoder(const SharedPtr<SynthEngine>& engine);

        // Name <-> enum helpers for scripting. Unknown names return the Count value.
        static Param ParamFromName(const char* name);
        static const char* ParamName(Param p);
        static float ValueFromName(Param p, const char* name); // "saw" for waves, "lowpass" for modes...
        static EffectType EffectFromName(const char* name);
        static i32 EffectParamFromName(EffectType type, const char* name);

        struct Impl;

    private:
        Impl* m_Impl = nullptr;
        u32 m_SampleRate;
        mutable Mutex m_Lock;
    };
}
