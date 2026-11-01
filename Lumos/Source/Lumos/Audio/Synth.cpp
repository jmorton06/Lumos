#include "Precompiled.h"
#include "Synth.h"
#include "AudioDecoder.h"
#include "WavWriter.h"
#include "Maths/MathsUtilities.h"

#include <cmath>
#include <cstring>

namespace Lumos
{
    using SE = SynthEngine;

    namespace
    {
        constexpr float TwoPi = 6.28318530718f;
        constexpr u32 Block   = SE::BlockFrames;

        inline float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
        inline float NoteHz(float note) { return 440.0f * exp2f((note - 69.0f) / 12.0f); }
        inline float DbToGain(float db) { return powf(10.0f, db / 20.0f); }

        struct Rng
        {
            u32 s = 0x9E3779B9u;
            inline u32 Next()
            {
                s ^= s << 13;
                s ^= s >> 17;
                s ^= s << 5;
                return s;
            }
            inline float Bipolar() { return (float)(Next() >> 8) * (2.0f / 16777216.0f) - 1.0f; }
            inline float Unit() { return (float)(Next() >> 8) * (1.0f / 16777216.0f); }
        };

        // polyBLEP band-limiting for the saw / square edges
        inline float PolyBlep(float t, float dt)
        {
            if(t < dt)
            {
                t /= dt;
                return t + t - t * t - 1.0f;
            }
            if(t > 1.0f - dt)
            {
                t = (t - 1.0f) / dt;
                return t * t + t + t + 1.0f;
            }
            return 0.0f;
        }

        // Zavalishin / Cytomic trapezoidal state-variable filter
        struct SVF
        {
            float ic1 = 0.0f, ic2 = 0.0f;
            float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, k = 2.0f;
            void Set(float cutoff, float res, float sr)
            {
                float g = tanf(3.14159265f * Clampf(cutoff, 20.0f, sr * 0.45f) / sr);
                k       = 2.0f - 1.96f * Clampf(res, 0.0f, 1.0f);
                a1      = 1.0f / (1.0f + g * (g + k));
                a2      = g * a1;
                a3      = g * a2;
            }
            inline float Process(float v0, u8 mode)
            {
                float v3 = v0 - ic2;
                float v1 = a1 * ic1 + a2 * v3;
                float v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1      = 2.0f * v1 - ic1;
                ic2      = 2.0f * v2 - ic2;
                if(mode == SE::Filter_HighPass)
                    return v0 - k * v1 - v2;
                if(mode == SE::Filter_BandPass)
                    return v1;
                return v2;
            }
        };

        struct Ramp
        {
            bool active = false;
            float from = 0.0f, to = 0.0f;
            u64 start = 0, end = 0;
        };

        enum EnvStage : u8
        {
            Env_Attack,
            Env_Decay,
            Env_Sustain,
            Env_Release,
            Env_Off
        };

        struct Env
        {
            float value = 0.0f;
            u8 stage    = Env_Off;
            float attackStep = 0.0f, decayCoef = 0.0f, releaseCoef = 0.0f, sustain = 1.0f;

            void Setup(float a, float d, float s, float r, float sr)
            {
                attackStep  = 1.0f / Maths::Max(1.0f, a * sr);
                // time constant a quarter of the stage time -> ~98% there at the end
                decayCoef   = 1.0f - expf(-4.0f / Maths::Max(1.0f, d * sr));
                releaseCoef = 1.0f - expf(-4.0f / Maths::Max(1.0f, r * sr));
                sustain     = Clampf(s, 0.0f, 1.0f);
            }
            inline float Tick()
            {
                switch(stage)
                {
                case Env_Attack:
                    value += attackStep;
                    if(value >= 1.0f)
                    {
                        value = 1.0f;
                        stage = Env_Decay;
                    }
                    break;
                case Env_Decay:
                    value += (sustain - value) * decayCoef;
                    if(fabsf(value - sustain) < 0.0005f)
                    {
                        value = sustain;
                        stage = Env_Sustain;
                    }
                    break;
                case Env_Sustain:
                    value = sustain;
                    break;
                case Env_Release:
                    value -= value * releaseCoef;
                    if(value < 0.0002f)
                    {
                        value = 0.0f;
                        stage = Env_Off;
                    }
                    break;
                default:
                    break;
                }
                return value;
            }
        };

        struct Voice
        {
            bool active   = false;
            bool released = false;
            float note = 60.0f, glideFrom = 60.0f, velocity = 1.0f;
            float phase1 = 0.0f, phase2 = 0.0f;
            float time   = 0.0f; // seconds since note on
            u64 age      = 0;
            Env amp, filt;
            SVF filter;
            float pink[7] = {};
            float brown   = 0.0f;
            float dust    = 0.0f;
            Rng rng;
            double samplePos = 0.0;
        };

        struct Sample
        {
            TDArray<float> data; // interleaved
            u32 channels = 1;
            u32 frames   = 0;
        };

        struct Instrument
        {
            bool alive = false;
            float params[SE::P_Count] = {};
            Ramp ramps[SE::P_Count];
            Voice voices[SE::MaxVoices];
            float lfoPhase = 0.0f;
            float lfoHold  = 0.0f; // sample & hold value for the random LFO
            float lastNote = -1.0f;
            u64 ageCounter = 0;
        };

        struct Effect
        {
            SE::EffectType type = SE::FX_Gain;
            bool enabled        = true;
            float p[SE::MaxEffectParams] = {};

            // state
            SVF svfL, svfR;
            float holdL = 0.0f, holdR = 0.0f;
            u32 holdCount = 0;
            TDArray<float> bufL, bufR; // delay / tape lines
            u32 writePos = 0;
            float lpL = 0.0f, lpR = 0.0f;
            float phaseA = 0.0f, phaseB = 0.0f;
            float env = 0.0f;
            // freeverb
            TDArray<float> comb[16];
            float combLp[16] = {};
            u32 combPos[16]  = {};
            TDArray<float> ap[8];
            u32 apPos[8] = {};
        };

        struct Bus
        {
            bool alive   = false;
            float volume = 1.0f, pan = 0.0f;
            i32 output   = 0;
            Ramp volRamp;
            TDArray<Effect*> effects;
            float l[Block] = {}, r[Block] = {};
            float peak = 0.0f;  // meter
            float key  = 0.0f;  // block peak, sidechain source
        };

        enum EventType : u8
        {
            Ev_NoteOn,
            Ev_NoteOff,
            Ev_Param,
            Ev_BusVolume
        };

        struct SynthEvent
        {
            u64 frame;
            EventType type;
            u8 param;
            i32 target;
            float a, b;
        };

        const char* s_ParamNames[SE::P_Count] = {
            "osc1", "osc1Level", "osc1Octave",
            "osc2", "osc2Level", "osc2Octave", "osc2Detune",
            "pulseWidth", "dustDensity",
            "pitchEnv", "pitchDecay",
            "attack", "decay", "sustain", "release",
            "filter", "cutoff", "resonance", "filterEnv", "keyTrack",
            "filterAttack", "filterDecay", "filterSustain", "filterRelease",
            "lfoTarget", "lfoWave", "lfoRate", "lfoDepth",
            "glide", "volume", "pan", "voices", "bus", "velocitySens", "tune",
            "sample", "sampleRoot", "sampleLoop", "sampleStart", "sampleEnd",
        };

        const char* s_WaveNames[SE::Wave_Count]     = { "sine", "saw", "square", "triangle", "noise", "pink", "brown", "dust" };
        const char* s_FilterNames[SE::Filter_Count] = { "off", "lowpass", "highpass", "bandpass" };
        const char* s_LfoNames[SE::Lfo_Count]       = { "off", "pitch", "cutoff", "amp", "pan" };
        const char* s_EffectNames[SE::FX_Count]     = { "filter", "bitcrush", "saturate", "delay", "reverb", "tape", "compressor", "gain" };
        const char* s_EffectParams[SE::FX_Count][SE::MaxEffectParams] = {
            { "mode", "cutoff", "resonance" },
            { "bits", "downsample", "mix" },
            { "drive", "mix" },
            { "time", "feedback", "mix", "pingpong", "damp" },
            { "size", "damp", "mix", "width" },
            { "wow", "flutter", "drive", "age" },
            { "threshold", "ratio", "attack", "release", "makeup", "sidechain" },
            { "gain" },
        };
        const float s_EffectDefaults[SE::FX_Count][SE::MaxEffectParams] = {
            { 1.0f, 2000.0f, 0.2f },
            { 8.0f, 4.0f, 1.0f },
            { 2.0f, 1.0f },
            { 0.35f, 0.35f, 0.25f, 0.0f, 0.3f },
            { 0.7f, 0.5f, 0.25f, 1.0f },
            { 0.3f, 0.2f, 1.2f, 0.3f },
            { -18.0f, 4.0f, 5.0f, 120.0f, 0.0f, -1.0f },
            { 0.0f },
        };

        void DefaultParams(float* p)
        {
            p[SE::P_Osc1Wave] = SE::Wave_Saw;
            p[SE::P_Osc1Level] = 1.0f;
            p[SE::P_Osc2Wave] = SE::Wave_Saw;
            p[SE::P_Osc2Level] = 0.0f;
            p[SE::P_PulseWidth] = 0.5f;
            p[SE::P_DustDensity] = 20.0f;
            p[SE::P_PitchDecay] = 0.08f;
            p[SE::P_Attack] = 0.005f;
            p[SE::P_Decay] = 0.2f;
            p[SE::P_Sustain] = 0.7f;
            p[SE::P_Release] = 0.25f;
            p[SE::P_FilterMode] = SE::Filter_LowPass;
            p[SE::P_Cutoff] = 8000.0f;
            p[SE::P_Resonance] = 0.1f;
            p[SE::P_FilterAttack] = 0.005f;
            p[SE::P_FilterDecay] = 0.3f;
            p[SE::P_FilterSustain] = 0.0f;
            p[SE::P_FilterRelease] = 0.3f;
            p[SE::P_LfoWave] = SE::Wave_Sine;
            p[SE::P_LfoRate] = 4.0f;
            p[SE::P_Volume] = 0.6f;
            p[SE::P_Voices] = 8.0f;
            p[SE::P_VelocitySens] = 0.7f;
            p[SE::P_Sample] = -1.0f;
            p[SE::P_SampleRoot] = 60.0f;
            p[SE::P_SampleEnd] = 1.0f;
        }

        void InitEffect(Effect* e, SE::EffectType type, float sr)
        {
            e->type = type;
            for(u32 i = 0; i < SE::MaxEffectParams; i++)
                e->p[i] = s_EffectDefaults[type][i];
            if(type == SE::FX_Delay)
            {
                e->bufL.Resize((u32)(sr * 2.0f) + 4, 0.0f);
                e->bufR.Resize((u32)(sr * 2.0f) + 4, 0.0f);
            }
            else if(type == SE::FX_Tape)
            {
                e->bufL.Resize((u32)(sr * 0.05f) + 4, 0.0f);
                e->bufR.Resize((u32)(sr * 0.05f) + 4, 0.0f);
            }
            else if(type == SE::FX_Reverb)
            {
                static const u32 combs[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
                static const u32 aps[4]   = { 556, 441, 341, 225 };
                float scale               = sr / 44100.0f;
                for(u32 i = 0; i < 8; i++)
                {
                    e->comb[i].Resize((u32)(combs[i] * scale) + 1, 0.0f);
                    e->comb[i + 8].Resize((u32)((combs[i] + 23) * scale) + 1, 0.0f);
                }
                for(u32 i = 0; i < 4; i++)
                {
                    e->ap[i].Resize((u32)(aps[i] * scale) + 1, 0.0f);
                    e->ap[i + 4].Resize((u32)((aps[i] + 23) * scale) + 1, 0.0f);
                }
            }
        }

        inline float ReadFrac(const TDArray<float>& buf, u32 writePos, float delay)
        {
            u32 n     = (u32)buf.Size();
            float pos = (float)writePos - delay;
            while(pos < 0.0f)
                pos += (float)n;
            u32 i0  = (u32)pos % n;
            u32 i1  = (i0 + 1) % n;
            float f = pos - floorf(pos);
            return buf[i0] + (buf[i1] - buf[i0]) * f;
        }
    }

    struct SynthEngine::Impl
    {
        float sr;
        u64 frame = 0;
        TDArray<Instrument*> instruments;
        TDArray<Bus*> buses;
        TDArray<Sample*> samples;
        TDArray<SynthEvent> events; // sorted by frame
        TDArray<float> capture;

        ~Impl()
        {
            for(auto* i : instruments)
                delete i;
            for(auto* b : buses)
            {
                if(!b)
                    continue;
                for(auto* e : b->effects)
                    delete e;
                delete b;
            }
            for(auto* s : samples)
                delete s;
        }

        Instrument* Inst(i32 id) { return (id >= 0 && id < (i32)instruments.Size() && instruments[id]->alive) ? instruments[id] : nullptr; }
        Bus* GetBus(i32 id) { return (id >= 0 && id < (i32)buses.Size() && buses[id]->alive) ? buses[id] : nullptr; }

        void Push(const SynthEvent& e)
        {
            // binary search for the insert point; equal frames keep insertion order
            u32 lo = 0, hi = (u32)events.Size();
            while(lo < hi)
            {
                u32 mid = (lo + hi) / 2;
                if(events[mid].frame <= e.frame)
                    lo = mid + 1;
                else
                    hi = mid;
            }
            events.PushBack(e);
            for(u32 i = (u32)events.Size() - 1; i > lo; i--)
                events[i] = events[i - 1];
            events[lo] = e;
        }

        u64 ToFrame(double time) const { return time <= 0.0 ? 0 : (u64)(time * sr); }

        // ---------------------------------------------------------------- voices

        void StartNote(Instrument* in, float note, float vel)
        {
            float* p     = in->params;
            u32 maxVoice = (u32)Clampf(p[SE::P_Voices], 1.0f, (float)SE::MaxVoices);
            Voice* v     = nullptr;
            if(maxVoice == 1)
                v = &in->voices[0]; // mono: retrigger, glide from the last note
            else
            {
                u64 oldest = ~0ull;
                for(u32 i = 0; i < maxVoice; i++)
                {
                    Voice& c = in->voices[i];
                    if(!c.active)
                    {
                        v = &c;
                        break;
                    }
                    // steal released voices first, then the oldest
                    u64 score = c.released ? c.age / 2 : c.age;
                    if(score < oldest)
                    {
                        oldest = score;
                        v      = &c;
                    }
                }
            }
            bool legato  = maxVoice == 1 && v->active && !v->released;
            v->glideFrom = (p[SE::P_Glide] > 0.0f && in->lastNote >= 0.0f) ? in->lastNote : note;
            v->note     = note;
            v->velocity = vel;
            v->active   = true;
            v->released = false;
            v->age      = ++in->ageCounter;
            v->time     = 0.0f;
            v->samplePos = 0.0;
            if(!legato)
            {
                v->phase1 = v->phase2 = 0.0f;
                v->amp.value  = 0.0f;
                v->filt.value = 0.0f;
                v->filter     = SVF();
            }
            v->amp.stage  = Env_Attack;
            v->filt.stage = Env_Attack;
            v->rng.s ^= (u32)(v->age * 2654435761u) | 1u;
            in->lastNote = note;

            i32 sampleId = (i32)p[SE::P_Sample];
            if(sampleId >= 0 && sampleId < (i32)samples.Size())
                v->samplePos = Clampf(p[SE::P_SampleStart], 0.0f, 1.0f) * samples[sampleId]->frames;
        }

        void ReleaseNote(Instrument* in, float note)
        {
            for(auto& v : in->voices)
                if(v.active && !v.released && fabsf(v.note - note) < 0.01f)
                {
                    v.released   = true;
                    v.amp.stage  = Env_Release;
                    v.filt.stage = Env_Release;
                }
        }

        float Osc(Voice& v, u8 wave, float& phase, float dt, float pw, float dustDensity)
        {
            float out = 0.0f;
            switch(wave)
            {
            case SE::Wave_Sine:
                out = sinf(phase * TwoPi);
                break;
            case SE::Wave_Saw:
                out = 2.0f * phase - 1.0f - PolyBlep(phase, dt);
                break;
            case SE::Wave_Square:
            {
                out = phase < pw ? 1.0f : -1.0f;
                out += PolyBlep(phase, dt);
                float t2 = phase - pw;
                if(t2 < 0.0f)
                    t2 += 1.0f;
                out -= PolyBlep(t2, dt);
                break;
            }
            case SE::Wave_Triangle:
                out = 4.0f * fabsf(phase - 0.5f) - 1.0f;
                break;
            case SE::Wave_Noise:
                return v.rng.Bipolar();
            case SE::Wave_Pink:
            {
                // Paul Kellet's filter
                float w = v.rng.Bipolar();
                float* b = v.pink;
                b[0] = 0.99886f * b[0] + w * 0.0555179f;
                b[1] = 0.99332f * b[1] + w * 0.0750759f;
                b[2] = 0.96900f * b[2] + w * 0.1538520f;
                b[3] = 0.86650f * b[3] + w * 0.3104856f;
                b[4] = 0.55000f * b[4] + w * 0.5329522f;
                b[5] = -0.7616f * b[5] - w * 0.0168980f;
                float pk = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362f;
                b[6] = w * 0.115926f;
                return pk * 0.11f;
            }
            case SE::Wave_Brown:
                v.brown = (v.brown + 0.02f * v.rng.Bipolar()) * 0.998f;
                return Clampf(v.brown * 3.5f, -1.0f, 1.0f);
            case SE::Wave_Dust:
                if(v.rng.Unit() < dustDensity / sr)
                    v.dust = (0.3f + 0.7f * v.rng.Unit()) * (v.rng.Next() & 1 ? 1.0f : -1.0f);
                out = v.dust;
                v.dust *= 0.6f;
                return out;
            default:
                break;
            }
            phase += dt;
            if(phase >= 1.0f)
                phase -= 1.0f;
            return out;
        }

        float Lfo(Instrument* in, u32 frames)
        {
            float* p = in->params;
            float ph = in->lfoPhase;
            float out;
            switch((u8)p[SE::P_LfoWave])
            {
            case SE::Wave_Saw: out = 2.0f * ph - 1.0f; break;
            case SE::Wave_Square: out = ph < 0.5f ? 1.0f : -1.0f; break;
            case SE::Wave_Triangle: out = 4.0f * fabsf(ph - 0.5f) - 1.0f; break;
            case SE::Wave_Noise: out = in->lfoHold; break;
            default: out = sinf(ph * TwoPi); break;
            }
            ph += p[SE::P_LfoRate] * frames / sr;
            if(ph >= 1.0f)
            {
                ph -= floorf(ph);
                Rng r;
                r.s = (u32)(frame * 747796405u) | 1u;
                in->lfoHold = r.Bipolar();
            }
            in->lfoPhase = ph;
            return out;
        }

        void RenderInstrument(Instrument* in, float* outL, float* outR, u32 n)
        {
            float* p        = in->params;
            u8 lfoTarget    = (u8)p[SE::P_LfoTarget];
            float lfo       = lfoTarget != SE::Lfo_Off ? Lfo(in, n) * p[SE::P_LfoDepth] : 0.0f;
            float vol       = p[SE::P_Volume];
            float panBase   = Clampf(p[SE::P_Pan] + (lfoTarget == SE::Lfo_Pan ? lfo : 0.0f), -1.0f, 1.0f);
            float ampMod    = lfoTarget == SE::Lfo_Amp ? Clampf(1.0f - (lfo * 0.5f + 0.5f * fabsf(p[SE::P_LfoDepth])), 0.0f, 1.0f) : 1.0f;
            float panL      = cosf((panBase + 1.0f) * 0.25f * 3.14159265f) * 1.41421356f;
            float panR      = sinf((panBase + 1.0f) * 0.25f * 3.14159265f) * 1.41421356f;
            u8 w1 = (u8)p[SE::P_Osc1Wave], w2 = (u8)p[SE::P_Osc2Wave];
            float l1 = p[SE::P_Osc1Level], l2 = p[SE::P_Osc2Level];
            float pw = Clampf(p[SE::P_PulseWidth], 0.05f, 0.95f);
            u8 fmode = (u8)p[SE::P_FilterMode];

            Sample* smp = nullptr;
            i32 sid     = (i32)p[SE::P_Sample];
            if(sid >= 0 && sid < (i32)samples.Size())
                smp = samples[sid];

            for(auto& v : in->voices)
            {
                if(!v.active)
                    continue;
                v.amp.Setup(p[SE::P_Attack], p[SE::P_Decay], p[SE::P_Sustain], p[SE::P_Release], sr);
                v.filt.Setup(p[SE::P_FilterAttack], p[SE::P_FilterDecay], p[SE::P_FilterSustain], p[SE::P_FilterRelease], sr);

                // pitch for this block: glide, pitch envelope, LFO
                float glideT = p[SE::P_Glide] > 0.0f ? Clampf(v.time / p[SE::P_Glide], 0.0f, 1.0f) : 1.0f;
                float note   = v.glideFrom + (v.note - v.glideFrom) * glideT + p[SE::P_Tune];
                if(p[SE::P_PitchEnv] != 0.0f)
                    note += p[SE::P_PitchEnv] * expf(-v.time * 5.0f / Maths::Max(0.001f, p[SE::P_PitchDecay]));
                if(lfoTarget == SE::Lfo_Pitch)
                    note += lfo;
                float f1  = NoteHz(note + 12.0f * p[SE::P_Osc1Octave]);
                float f2  = NoteHz(note + 12.0f * p[SE::P_Osc2Octave] + p[SE::P_Osc2Detune]);
                float dt1 = Clampf(f1 / sr, 0.0f, 0.49f), dt2 = Clampf(f2 / sr, 0.0f, 0.49f);

                // filter cutoff follows the envelope at control rate
                if(fmode != SE::Filter_Off)
                {
                    float oct = p[SE::P_FilterEnv] * v.filt.value + p[SE::P_KeyTrack] * (v.note - 60.0f) / 12.0f;
                    if(lfoTarget == SE::Lfo_Cutoff)
                        oct += lfo;
                    v.filter.Set(p[SE::P_Cutoff] * exp2f(oct), p[SE::P_Resonance], sr);
                }

                float velGain = 1.0f - p[SE::P_VelocitySens] * (1.0f - v.velocity);
                float gain    = vol * velGain * ampMod;
                double rate   = smp ? exp2(((double)note - p[SE::P_SampleRoot]) / 12.0) : 0.0;
                double endF   = smp ? Clampf(p[SE::P_SampleEnd], 0.0f, 1.0f) * smp->frames : 0.0;
                double startF = smp ? Clampf(p[SE::P_SampleStart], 0.0f, 1.0f) * smp->frames : 0.0;
                bool loop     = p[SE::P_SampleLoop] > 0.5f;

                for(u32 i = 0; i < n; i++)
                {
                    float sL, sR;
                    if(smp)
                    {
                        if(v.samplePos >= endF - 1.0)
                        {
                            if(loop && endF - startF > 2.0)
                                v.samplePos = startF + fmod(v.samplePos - startF, endF - startF);
                            else
                            {
                                v.active = false;
                                break;
                            }
                        }
                        u32 i0  = (u32)v.samplePos;
                        float f = (float)(v.samplePos - i0);
                        u32 i1  = Maths::Min(i0 + 1, smp->frames - 1);
                        const float* d = smp->data.Data();
                        if(smp->channels == 2)
                        {
                            sL = d[i0 * 2] + (d[i1 * 2] - d[i0 * 2]) * f;
                            sR = d[i0 * 2 + 1] + (d[i1 * 2 + 1] - d[i0 * 2 + 1]) * f;
                        }
                        else
                            sL = sR = d[i0] + (d[i1] - d[i0]) * f;
                        v.samplePos += rate;
                    }
                    else
                    {
                        float s = 0.0f;
                        if(l1 != 0.0f)
                            s += l1 * Osc(v, w1, v.phase1, dt1, pw, p[SE::P_DustDensity]);
                        if(l2 != 0.0f)
                            s += l2 * Osc(v, w2, v.phase2, dt2, pw, p[SE::P_DustDensity]);
                        sL = sR = s;
                    }
                    if(fmode != SE::Filter_Off)
                    {
                        if(smp && smp->channels == 2)
                        {
                            // stereo samples: filter the mid, keep the side
                            float mid = (sL + sR) * 0.5f, side = (sL - sR) * 0.5f;
                            mid = v.filter.Process(mid, fmode);
                            sL  = mid + side;
                            sR  = mid - side;
                        }
                        else
                            sL = sR = v.filter.Process(sL, fmode);
                    }
                    v.filt.Tick();
                    float a = v.amp.Tick() * gain;
                    outL[i] += sL * a * panL;
                    outR[i] += sR * a * panR;
                    if(v.amp.stage == Env_Off)
                    {
                        v.active = false;
                        break;
                    }
                }
                v.time += (float)n / sr;
            }
        }

        // ---------------------------------------------------------------- effects

        void ProcessEffect(Effect* e, float* L, float* R, u32 n)
        {
            float* p = e->p;
            switch(e->type)
            {
            case SE::FX_Filter:
            {
                u8 mode = (u8)p[0];
                if(mode == SE::Filter_Off)
                    break;
                e->svfL.Set(p[1], p[2], sr);
                e->svfR.Set(p[1], p[2], sr);
                for(u32 i = 0; i < n; i++)
                {
                    L[i] = e->svfL.Process(L[i], mode);
                    R[i] = e->svfR.Process(R[i], mode);
                }
                break;
            }
            case SE::FX_Bitcrush:
            {
                float levels = exp2f(Clampf(p[0], 1.0f, 16.0f) - 1.0f);
                u32 hold     = (u32)Clampf(p[1], 1.0f, 64.0f);
                float mix    = Clampf(p[2], 0.0f, 1.0f);
                for(u32 i = 0; i < n; i++)
                {
                    if(e->holdCount++ % hold == 0)
                    {
                        e->holdL = floorf(L[i] * levels + 0.5f) / levels;
                        e->holdR = floorf(R[i] * levels + 0.5f) / levels;
                    }
                    L[i] += (e->holdL - L[i]) * mix;
                    R[i] += (e->holdR - R[i]) * mix;
                }
                break;
            }
            case SE::FX_Saturate:
            {
                float drive = Maths::Max(0.1f, p[0]);
                float norm  = 1.0f / tanhf(drive);
                float mix   = Clampf(p[1], 0.0f, 1.0f);
                for(u32 i = 0; i < n; i++)
                {
                    L[i] += (tanhf(L[i] * drive) * norm - L[i]) * mix;
                    R[i] += (tanhf(R[i] * drive) * norm - R[i]) * mix;
                }
                break;
            }
            case SE::FX_Delay:
            {
                u32 size    = (u32)e->bufL.Size();
                float delay = Clampf(p[0] * sr, 1.0f, (float)size - 2.0f);
                float fb    = Clampf(p[1], 0.0f, 0.97f);
                float mix   = Clampf(p[2], 0.0f, 1.0f);
                bool ping   = p[3] > 0.5f;
                float damp  = Clampf(p[4], 0.0f, 0.99f);
                for(u32 i = 0; i < n; i++)
                {
                    float dl = ReadFrac(e->bufL, e->writePos, delay);
                    float dr = ReadFrac(e->bufR, e->writePos, delay);
                    e->lpL += (dl - e->lpL) * (1.0f - damp);
                    e->lpR += (dr - e->lpR) * (1.0f - damp);
                    float inL = L[i], inR = R[i];
                    if(ping)
                    {
                        e->bufL[e->writePos] = (inL + inR) * 0.5f + e->lpR * fb;
                        e->bufR[e->writePos] = e->lpL * fb;
                    }
                    else
                    {
                        e->bufL[e->writePos] = inL + e->lpL * fb;
                        e->bufR[e->writePos] = inR + e->lpR * fb;
                    }
                    e->writePos = (e->writePos + 1) % size;
                    L[i]        = inL + dl * mix;
                    R[i]        = inR + dr * mix;
                }
                break;
            }
            case SE::FX_Reverb:
            {
                float room  = Clampf(p[0], 0.0f, 1.0f) * 0.28f + 0.7f;
                float damp  = Clampf(p[1], 0.0f, 1.0f) * 0.4f;
                float mix   = Clampf(p[2], 0.0f, 1.0f);
                float width = Clampf(p[3], 0.0f, 1.0f);
                float wet1 = mix * (width * 0.5f + 0.5f), wet2 = mix * ((1.0f - width) * 0.5f);
                for(u32 i = 0; i < n; i++)
                {
                    float in    = (L[i] + R[i]) * 0.015f;
                    float outL = 0.0f, outR = 0.0f;
                    for(u32 c = 0; c < 16; c++)
                    {
                        TDArray<float>& buf = e->comb[c];
                        u32& pos            = e->combPos[c];
                        float o             = buf[pos];
                        e->combLp[c]        = o * (1.0f - damp) + e->combLp[c] * damp;
                        buf[pos]            = in + e->combLp[c] * room;
                        pos                 = (pos + 1) % (u32)buf.Size();
                        if(c < 8)
                            outL += o;
                        else
                            outR += o;
                    }
                    for(u32 a = 0; a < 8; a++)
                    {
                        TDArray<float>& buf = e->ap[a];
                        u32& pos            = e->apPos[a];
                        float b             = buf[pos];
                        float& x            = a < 4 ? outL : outR;
                        buf[pos]            = x + b * 0.5f;
                        x                   = b - x;
                        pos                 = (pos + 1) % (u32)buf.Size();
                    }
                    L[i] = L[i] * (1.0f - mix * 0.5f) + outL * wet1 + outR * wet2;
                    R[i] = R[i] * (1.0f - mix * 0.5f) + outR * wet1 + outL * wet2;
                }
                break;
            }
            case SE::FX_Tape:
            {
                u32 size    = (u32)e->bufL.Size();
                float wow   = Clampf(p[0], 0.0f, 1.0f);
                float flut  = Clampf(p[1], 0.0f, 1.0f);
                float drive = Maths::Max(0.1f, p[2]);
                float norm  = 1.0f / tanhf(drive);
                float age   = Clampf(p[3], 0.0f, 1.0f);
                float lpK   = 1.0f - expf(-TwoPi * (18000.0f - 15000.0f * age) / sr);
                float base  = 0.008f * sr;
                for(u32 i = 0; i < n; i++)
                {
                    e->bufL[e->writePos] = L[i];
                    e->bufR[e->writePos] = R[i];
                    float mod            = wow * 0.004f * sr * sinf(e->phaseA * TwoPi) + flut * 0.0004f * sr * sinf(e->phaseB * TwoPi);
                    float dl             = ReadFrac(e->bufL, e->writePos, base + mod);
                    float dr             = ReadFrac(e->bufR, e->writePos, base + mod);
                    e->writePos          = (e->writePos + 1) % size;
                    e->phaseA += 0.55f / sr;
                    e->phaseB += 6.5f / sr;
                    if(e->phaseA >= 1.0f)
                        e->phaseA -= 1.0f;
                    if(e->phaseB >= 1.0f)
                        e->phaseB -= 1.0f;
                    e->lpL += (tanhf(dl * drive) * norm - e->lpL) * lpK;
                    e->lpR += (tanhf(dr * drive) * norm - e->lpR) * lpK;
                    L[i] = e->lpL;
                    R[i] = e->lpR;
                }
                break;
            }
            case SE::FX_Compressor:
            {
                float thresh  = p[0];
                float ratio   = Maths::Max(1.0f, p[1]);
                float att     = 1.0f - expf(-1.0f / Maths::Max(1.0f, p[2] * 0.001f * sr));
                float rel     = 1.0f - expf(-1.0f / Maths::Max(1.0f, p[3] * 0.001f * sr));
                float makeup  = DbToGain(p[4]);
                Bus* side     = p[5] >= 0.0f ? GetBus((i32)p[5]) : nullptr;
                for(u32 i = 0; i < n; i++)
                {
                    float key = side ? side->key : Maths::Max(fabsf(L[i]), fabsf(R[i]));
                    e->env += (key - e->env) * (key > e->env ? att : rel);
                    float db   = 20.0f * log10f(Maths::Max(e->env, 1e-6f));
                    float over = db - thresh;
                    float g    = over > 0.0f ? DbToGain(-over * (1.0f - 1.0f / ratio)) : 1.0f;
                    L[i] *= g * makeup;
                    R[i] *= g * makeup;
                }
                break;
            }
            case SE::FX_Gain:
            {
                float g = DbToGain(p[0]);
                for(u32 i = 0; i < n; i++)
                {
                    L[i] *= g;
                    R[i] *= g;
                }
                break;
            }
            default:
                break;
            }
        }

        // ---------------------------------------------------------------- block

        void ApplyEvents(u64 blockEnd)
        {
            u32 consumed = 0;
            while(consumed < events.Size() && events[consumed].frame < blockEnd)
            {
                const SynthEvent& e = events[consumed++];
                switch(e.type)
                {
                case Ev_NoteOn:
                    if(auto* in = Inst(e.target))
                        StartNote(in, e.a, e.b);
                    break;
                case Ev_NoteOff:
                    if(auto* in = Inst(e.target))
                        ReleaseNote(in, e.a);
                    break;
                case Ev_Param:
                    if(auto* in = Inst(e.target))
                    {
                        Ramp& r = in->ramps[e.param];
                        if(e.b <= 0.0f)
                        {
                            r.active            = false;
                            in->params[e.param] = e.a;
                        }
                        else
                        {
                            r.active = true;
                            r.from   = in->params[e.param];
                            r.to     = e.a;
                            r.start  = frame;
                            r.end    = frame + (u64)(e.b * sr);
                        }
                    }
                    break;
                case Ev_BusVolume:
                    if(auto* b = GetBus(e.target))
                    {
                        if(e.b <= 0.0f)
                        {
                            b->volRamp.active = false;
                            b->volume         = e.a;
                        }
                        else
                        {
                            b->volRamp = { true, b->volume, e.a, frame, frame + (u64)(e.b * sr) };
                        }
                    }
                    break;
                }
            }
            if(consumed > 0)
            {
                u32 left = (u32)events.Size() - consumed;
                for(u32 i = 0; i < left; i++)
                    events[i] = events[i + consumed];
                events.Resize(left);
            }
        }

        static float RampValue(const Ramp& r, u64 now)
        {
            if(now >= r.end)
                return r.to;
            float t = (float)(now - r.start) / (float)Maths::Max<u64>(1, r.end - r.start);
            return r.from + (r.to - r.from) * t;
        }

        void RenderBlock(float* out, u32 n)
        {
            ApplyEvents(frame + n);

            for(auto* b : buses)
            {
                memset(b->l, 0, sizeof(float) * n);
                memset(b->r, 0, sizeof(float) * n);
                if(b->volRamp.active)
                {
                    b->volume = RampValue(b->volRamp, frame);
                    if(frame >= b->volRamp.end)
                        b->volRamp.active = false;
                }
            }

            for(auto* in : instruments)
            {
                if(!in->alive)
                    continue;
                for(u32 p = 0; p < SE::P_Count; p++)
                {
                    Ramp& r = in->ramps[p];
                    if(!r.active)
                        continue;
                    in->params[p] = RampValue(r, frame);
                    if(frame >= r.end)
                        r.active = false;
                }
                Bus* b = GetBus((i32)in->params[SE::P_Bus]);
                if(!b)
                    b = buses[0];
                RenderInstrument(in, b->l, b->r, n);
            }

            // children first, into their output (lower index, else the master)
            for(i32 i = (i32)buses.Size() - 1; i >= 0; i--)
            {
                Bus* b = buses[i];
                if(!b->alive)
                    continue;
                for(auto* e : b->effects)
                    if(e->enabled)
                        ProcessEffect(e, b->l, b->r, n);
                float panL = Maths::Min(1.0f, 1.0f - b->pan), panR = Maths::Min(1.0f, 1.0f + b->pan);
                float peak = 0.0f;
                for(u32 f = 0; f < n; f++)
                {
                    b->l[f] *= b->volume * panL;
                    b->r[f] *= b->volume * panR;
                    peak = Maths::Max(peak, Maths::Max(fabsf(b->l[f]), fabsf(b->r[f])));
                }
                b->key  = peak;
                b->peak = Maths::Max(peak, b->peak * 0.97f);
                if(i == 0)
                    break;
                Bus* dst = (b->output >= 0 && b->output < i) ? GetBus(b->output) : nullptr;
                if(!dst)
                    dst = buses[0];
                for(u32 f = 0; f < n; f++)
                {
                    dst->l[f] += b->l[f];
                    dst->r[f] += b->r[f];
                }
            }

            // soft knee above 0.8 so a hot mix bends into +-1 instead of wrapping
            auto knee = [](float x)
            {
                float a = fabsf(x);
                if(a <= 0.8f)
                    return x;
                float y = 0.8f + 0.2f * tanhf((a - 0.8f) / 0.2f);
                return x < 0.0f ? -y : y;
            };
            Bus* m = buses[0];
            for(u32 f = 0; f < n; f++)
            {
                out[f * 2]     = knee(m->l[f]);
                out[f * 2 + 1] = knee(m->r[f]);
            }
            frame += n;
        }
    };

    // ---------------------------------------------------------------- decoder adapter

    namespace
    {
        class SynthDecoder : public AudioDecoder
        {
        public:
            explicit SynthDecoder(const SharedPtr<SynthEngine>& engine)
                : m_Engine(engine)
            {
                m_SampleRate  = engine->GetSampleRate();
                m_Channels    = 2;
                m_TotalFrames = 0;
            }

            u32 Read(i16* out, u32 frames) override
            {
                u32 done = 0;
                while(done < frames)
                {
                    u32 n = Maths::Min<u32>(frames - done, 1024);
                    m_Engine->RenderFloat(m_Buf, n);
                    for(u32 i = 0; i < n * 2; i++)
                        out[done * 2 + i] = (i16)(Clampf(m_Buf[i], -1.0f, 1.0f) * 32767.0f);
                    done += n;
                }
                return frames;
            }

            bool Seek(u64) override { return true; }

        private:
            SharedPtr<SynthEngine> m_Engine;
            float m_Buf[2048];
        };
    }

    // ---------------------------------------------------------------- public API

    SynthEngine::SynthEngine(u32 sampleRate)
        : m_SampleRate(sampleRate)
    {
        MutexInit(&m_Lock);
        m_Impl     = new Impl();
        m_Impl->sr = (float)sampleRate;
        AddBus(); // master
    }

    SynthEngine::~SynthEngine()
    {
        delete m_Impl;
        MutexDestroy(&m_Lock);
    }

    double SynthEngine::Now() const
    {
        ScopedMutex lock(&m_Lock);
        return (double)m_Impl->frame / m_SampleRate;
    }

    i32 SynthEngine::AddInstrument()
    {
        ScopedMutex lock(&m_Lock);
        auto* in  = new Instrument();
        in->alive = true;
        DefaultParams(in->params);
        m_Impl->instruments.PushBack(in);
        return (i32)m_Impl->instruments.Size() - 1;
    }

    void SynthEngine::RemoveInstrument(i32 inst)
    {
        ScopedMutex lock(&m_Lock);
        if(auto* in = m_Impl->Inst(inst))
        {
            in->alive = false;
            for(auto& v : in->voices)
                v.active = false;
        }
    }

    void SynthEngine::SetParam(i32 inst, Param p, float value)
    {
        if(p >= P_Count)
            return;
        ScopedMutex lock(&m_Lock);
        if(auto* in = m_Impl->Inst(inst))
        {
            in->ramps[p].active = false;
            in->params[p]       = value;
        }
    }

    void SynthEngine::ScheduleParam(i32 inst, Param p, float value, double time, float rampSeconds)
    {
        if(p >= P_Count)
            return;
        ScopedMutex lock(&m_Lock);
        m_Impl->Push({ m_Impl->ToFrame(time), Ev_Param, (u8)p, inst, value, rampSeconds });
    }

    float SynthEngine::GetParam(i32 inst, Param p) const
    {
        ScopedMutex lock(&m_Lock);
        auto* in = m_Impl->Inst(inst);
        return (in && p < P_Count) ? in->params[p] : 0.0f;
    }

    void SynthEngine::NoteOn(i32 inst, float note, float velocity, double time, float duration)
    {
        ScopedMutex lock(&m_Lock);
        u64 f = m_Impl->ToFrame(time);
        m_Impl->Push({ f, Ev_NoteOn, 0, inst, note, Clampf(velocity, 0.0f, 1.0f) });
        if(duration > 0.0f)
            m_Impl->Push({ f + Maths::Max<u64>(1, (u64)(duration * m_SampleRate)), Ev_NoteOff, 0, inst, note, 0.0f });
    }

    void SynthEngine::NoteOff(i32 inst, float note, double time)
    {
        ScopedMutex lock(&m_Lock);
        m_Impl->Push({ m_Impl->ToFrame(time), Ev_NoteOff, 0, inst, note, 0.0f });
    }

    void SynthEngine::AllNotesOff(i32 inst)
    {
        ScopedMutex lock(&m_Lock);
        for(i32 i = 0; i < (i32)m_Impl->instruments.Size(); i++)
        {
            if(inst >= 0 && i != inst)
                continue;
            for(auto& v : m_Impl->instruments[i]->voices)
                if(v.active)
                {
                    v.released  = true;
                    v.amp.stage = Env_Release;
                }
        }
    }

    void SynthEngine::ClearScheduled(i32 inst)
    {
        ScopedMutex lock(&m_Lock);
        auto& ev = m_Impl->events;
        if(inst < 0)
        {
            ev.Clear();
            return;
        }
        ev.RemoveIf([inst](const SynthEvent& e) { return e.target == inst && e.type != Ev_BusVolume; });
    }

    i32 SynthEngine::AddBus()
    {
        ScopedMutex lock(&m_Lock);
        auto* b  = new Bus();
        b->alive = true;
        m_Impl->buses.PushBack(b);
        return (i32)m_Impl->buses.Size() - 1;
    }

    void SynthEngine::SetBusVolume(i32 bus, float volume)
    {
        ScopedMutex lock(&m_Lock);
        if(auto* b = m_Impl->GetBus(bus))
        {
            b->volRamp.active = false;
            b->volume         = Maths::Max(0.0f, volume);
        }
    }

    void SynthEngine::SetBusPan(i32 bus, float pan)
    {
        ScopedMutex lock(&m_Lock);
        if(auto* b = m_Impl->GetBus(bus))
            b->pan = Clampf(pan, -1.0f, 1.0f);
    }

    void SynthEngine::SetBusOutput(i32 bus, i32 output)
    {
        ScopedMutex lock(&m_Lock);
        if(auto* b = m_Impl->GetBus(bus))
            b->output = output;
    }

    void SynthEngine::ScheduleBusVolume(i32 bus, float volume, double time, float rampSeconds)
    {
        ScopedMutex lock(&m_Lock);
        m_Impl->Push({ m_Impl->ToFrame(time), Ev_BusVolume, 0, bus, Maths::Max(0.0f, volume), rampSeconds });
    }

    float SynthEngine::GetBusLevel(i32 bus) const
    {
        ScopedMutex lock(&m_Lock);
        auto* b = m_Impl->GetBus(bus);
        return b ? b->peak : 0.0f;
    }

    i32 SynthEngine::AddEffect(i32 bus, EffectType type)
    {
        if(type >= FX_Count)
            return -1;
        // delay lines are allocated before taking the lock so the audio thread never waits on it
        auto* e = new Effect();
        InitEffect(e, type, (float)m_SampleRate);
        ScopedMutex lock(&m_Lock);
        auto* b = m_Impl->GetBus(bus);
        if(!b)
        {
            delete e;
            return -1;
        }
        b->effects.PushBack(e);
        return (i32)b->effects.Size() - 1;
    }

    void SynthEngine::SetEffectParam(i32 bus, i32 effect, u32 param, float value)
    {
        if(param >= MaxEffectParams)
            return;
        ScopedMutex lock(&m_Lock);
        auto* b = m_Impl->GetBus(bus);
        if(b && effect >= 0 && effect < (i32)b->effects.Size())
            b->effects[effect]->p[param] = value;
    }

    void SynthEngine::SetEffectEnabled(i32 bus, i32 effect, bool enabled)
    {
        ScopedMutex lock(&m_Lock);
        auto* b = m_Impl->GetBus(bus);
        if(b && effect >= 0 && effect < (i32)b->effects.Size())
            b->effects[effect]->enabled = enabled;
    }

    void SynthEngine::ClearEffects(i32 bus)
    {
        TDArray<Effect*> dead;
        {
            ScopedMutex lock(&m_Lock);
            auto* b = m_Impl->GetBus(bus);
            if(!b)
                return;
            dead = b->effects;
            b->effects.Clear();
        }
        for(auto* e : dead)
            delete e;
    }

    i32 SynthEngine::AddSample(const float* interleaved, u32 frames, u32 channels, u32 sampleRate)
    {
        if(!interleaved || frames == 0 || channels == 0)
            return -1;
        channels  = Maths::Min(channels, 2u);
        auto* s   = new Sample();
        s->channels = channels;
        // linear resample to the engine rate
        double ratio = (double)sampleRate / m_SampleRate;
        u32 outFrames = (u32)((double)frames / ratio);
        s->frames     = Maths::Max(1u, outFrames);
        s->data.Resize((size_t)s->frames * channels);
        for(u32 i = 0; i < s->frames; i++)
        {
            double src = i * ratio;
            u32 i0     = Maths::Min((u32)src, frames - 1);
            u32 i1     = Maths::Min(i0 + 1, frames - 1);
            float f    = (float)(src - i0);
            for(u32 c = 0; c < channels; c++)
                s->data[(size_t)i * channels + c] = interleaved[(size_t)i0 * channels + c] * (1.0f - f) + interleaved[(size_t)i1 * channels + c] * f;
        }
        ScopedMutex lock(&m_Lock);
        m_Impl->samples.PushBack(s);
        return (i32)m_Impl->samples.Size() - 1;
    }

    i32 SynthEngine::LoadSample(const std::string& path, float maxSeconds)
    {
        AudioDecoder* dec = AudioDecoder::Open(path);
        if(!dec)
            return -1;
        u32 ch       = dec->GetChannels();
        u32 rate     = dec->GetSampleRate();
        u32 maxFrames = (u32)(Maths::Max(0.1f, maxSeconds) * rate);
        TDArray<i16> raw;
        raw.Resize((size_t)maxFrames * ch);
        u32 got = 0;
        while(got < maxFrames)
        {
            u32 n = dec->Read(raw.Data() + (size_t)got * ch, Maths::Min(4096u, maxFrames - got));
            if(n == 0)
                break;
            got += n;
        }
        delete dec;
        if(got == 0 || ch == 0)
            return -1;
        u32 outCh = Maths::Min(ch, 2u);
        TDArray<float> f;
        f.Resize((size_t)got * outCh);
        for(u32 i = 0; i < got; i++)
            for(u32 c = 0; c < outCh; c++)
                f[(size_t)i * outCh + c] = raw[(size_t)i * ch + c] / 32768.0f;
        return AddSample(f.Data(), got, outCh, rate);
    }

    float SynthEngine::GetSampleLength(i32 sample) const
    {
        ScopedMutex lock(&m_Lock);
        if(sample < 0 || sample >= (i32)m_Impl->samples.Size())
            return 0.0f;
        return (float)m_Impl->samples[sample]->frames / m_SampleRate;
    }

    void SynthEngine::RenderFloat(float* stereo, u32 frames)
    {
        ScopedMutex lock(&m_Lock);
        u32 done = 0;
        while(done < frames)
        {
            u32 n = Maths::Min(frames - done, Block);
            m_Impl->RenderBlock(stereo + done * 2, n);
            done += n;
        }
    }

    void SynthEngine::Render(double seconds)
    {
        u32 frames = (u32)(Maths::Max(0.0, seconds) * m_SampleRate);
        size_t at  = m_Impl->capture.Size();
        m_Impl->capture.Resize(at + (size_t)frames * 2, 0.0f);
        u32 done = 0;
        while(done < frames)
        {
            u32 n = Maths::Min(frames - done, 4096u);
            RenderFloat(m_Impl->capture.Data() + at + (size_t)done * 2, n);
            done += n;
        }
    }

    bool SynthEngine::SaveWav(const std::string& path, bool clearCapture)
    {
        bool ok = WriteWav16(path, m_Impl->capture.Data(), (u32)(m_Impl->capture.Size() / 2), 2, m_SampleRate);
        if(ok && clearCapture)
            ClearCapture();
        return ok;
    }

    double SynthEngine::GetCaptureLength() const { return (double)(m_Impl->capture.Size() / 2) / m_SampleRate; }
    void SynthEngine::ClearCapture() { m_Impl->capture.Clear(); }

    AudioDecoder* SynthEngine::CreateDecoder(const SharedPtr<SynthEngine>& engine)
    {
        return engine ? new SynthDecoder(engine) : nullptr;
    }

    // ---------------------------------------------------------------- names

    SynthEngine::Param SynthEngine::ParamFromName(const char* name)
    {
        for(u32 i = 0; i < P_Count; i++)
            if(strcmp(name, s_ParamNames[i]) == 0)
                return (Param)i;
        return P_Count;
    }

    const char* SynthEngine::ParamName(Param p) { return p < P_Count ? s_ParamNames[p] : ""; }

    float SynthEngine::ValueFromName(Param p, const char* name)
    {
        auto find = [name](const char* const* names, u32 count) -> float
        {
            for(u32 i = 0; i < count; i++)
                if(strcmp(name, names[i]) == 0)
                    return (float)i;
            return -1.0f;
        };
        switch(p)
        {
        case P_Osc1Wave:
        case P_Osc2Wave:
        case P_LfoWave: return find(s_WaveNames, Wave_Count);
        case P_FilterMode: return find(s_FilterNames, Filter_Count);
        case P_LfoTarget: return find(s_LfoNames, Lfo_Count);
        default: return -1.0f;
        }
    }

    SynthEngine::EffectType SynthEngine::EffectFromName(const char* name)
    {
        for(u32 i = 0; i < FX_Count; i++)
            if(strcmp(name, s_EffectNames[i]) == 0)
                return (EffectType)i;
        return FX_Count;
    }

    i32 SynthEngine::EffectParamFromName(EffectType type, const char* name)
    {
        if(type >= FX_Count)
            return -1;
        for(u32 i = 0; i < MaxEffectParams; i++)
            if(s_EffectParams[type][i] && strcmp(name, s_EffectParams[type][i]) == 0)
                return (i32)i;
        return -1;
    }
}
