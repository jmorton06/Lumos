#include "Precompiled.h"
#include "AudioAnalysis.h"
#include "AudioDecoder.h"
#include "Core/Mutex.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/MathsUtilities.h"

#include <atomic>
#include <thread>
#include <cmath>
#include <unordered_map>

namespace Lumos
{
    namespace
    {
        constexpr u32 TargetRate = 11025;
        constexpr u32 FFTSize    = 1024;
        constexpr u32 Hop        = 256;

        std::atomic<bool> s_Stop { false };

        void FFT(float* re, float* im, u32 n)
        {
            for(u32 i = 1, j = 0; i < n; i++)
            {
                u32 bit = n >> 1;
                for(; j & bit; bit >>= 1)
                    j ^= bit;
                j ^= bit;
                if(i < j)
                {
                    std::swap(re[i], re[j]);
                    std::swap(im[i], im[j]);
                }
            }
            for(u32 len = 2; len <= n; len <<= 1)
            {
                float ang = -2.0f * 3.14159265f / (float)len;
                float wr = cosf(ang), wi = sinf(ang);
                for(u32 i = 0; i < n; i += len)
                {
                    float cr = 1.0f, ci = 0.0f;
                    for(u32 k = 0; k < len / 2; k++)
                    {
                        u32 a = i + k, b = i + k + len / 2;
                        float tr = re[b] * cr - im[b] * ci;
                        float ti = re[b] * ci + im[b] * cr;
                        re[b]    = re[a] - tr;
                        im[b]    = im[a] - ti;
                        re[a] += tr;
                        im[a] += ti;
                        float nr = cr * wr - ci * wi;
                        ci       = cr * wi + ci * wr;
                        cr       = nr;
                    }
                }
            }
        }

        // Krumhansl-Kessler key profiles
        const float s_Major[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
        const float s_Minor[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

        float Correlate(const float* a, const float* b, u32 shift)
        {
            float ma = 0.0f, mb = 0.0f;
            for(u32 i = 0; i < 12; i++)
            {
                ma += a[i];
                mb += b[i];
            }
            ma /= 12.0f;
            mb /= 12.0f;
            float num = 0.0f, da = 0.0f, db = 0.0f;
            for(u32 i = 0; i < 12; i++)
            {
                float x = a[(i + shift) % 12] - ma;
                float y = b[i] - mb;
                num += x * y;
                da += x * x;
                db += y * y;
            }
            return num / Maths::Max(1e-9f, sqrtf(da * db));
        }
    }

    namespace
    {
        // ITU-R BS.1770 K-weighting: high shelf + RLB high-pass, coefficients for any rate (as libebur128)
        struct Biquad
        {
            double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
            double z1 = 0, z2 = 0;
            inline double Run(double x)
            {
                double y = b0 * x + z1;
                z1       = b1 * x - a1 * y + z2;
                z2       = b2 * x - a2 * y;
                return y;
            }
        };

        void KWeighting(double fs, Biquad& shelf, Biquad& hp)
        {
            double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            double K  = tan(3.14159265358979 * f0 / fs);
            double Vh = pow(10.0, G / 20.0), Vb = pow(Vh, 0.4996667741545416);
            double a0 = 1.0 + K / Q + K * K;
            shelf.b0  = (Vh + Vb * K / Q + K * K) / a0;
            shelf.b1  = 2.0 * (K * K - Vh) / a0;
            shelf.b2  = (Vh - Vb * K / Q + K * K) / a0;
            shelf.a1  = 2.0 * (K * K - 1.0) / a0;
            shelf.a2  = (1.0 - K / Q + K * K) / a0;
            f0        = 38.13547087602444;
            Q         = 0.5003270373238773;
            K         = tan(3.14159265358979 * f0 / fs);
            a0        = 1.0 + K / Q + K * K;
            hp.b0 = 1.0, hp.b1 = -2.0, hp.b2 = 1.0;
            hp.a1 = 2.0 * (K * K - 1.0) / a0;
            hp.a2 = (1.0 - K / Q + K * K) / a0;
        }

        // Integrated loudness (400ms blocks, 75% overlap, -70 / -10 gates) and loudness range (3s, -70 / -20, P10-P95)
        struct LoudnessMeter
        {
            Biquad shelf[2], hp[2];
            u32 channels = 1, subLen = 0, subCount = 0;
            double subSum = 0.0;
            TDArray<double> subs; // mean-square energy per 100ms sub-block (channels summed)

            void Init(u32 rate, u32 ch)
            {
                channels = Maths::Min(ch, 2u);
                subLen   = Maths::Max(1u, rate / 10);
                for(u32 c = 0; c < 2; c++)
                    KWeighting((double)rate, shelf[c], hp[c]);
            }
            inline void Push(const float* frame)
            {
                double e = 0.0;
                for(u32 c = 0; c < channels; c++)
                {
                    double y = hp[c].Run(shelf[c].Run(frame[c]));
                    e += y * y;
                }
                subSum += e;
                if(++subCount == subLen)
                {
                    subs.PushBack(subSum / subLen);
                    subSum   = 0.0;
                    subCount = 0;
                }
            }
            static double Lufs(double e) { return e > 0.0 ? -0.691 + 10.0 * log10(e) : -120.0; }

            // blocks of `len` sub-blocks every `step`
            void Blocks(u32 len, u32 step, TDArray<double>& out) const
            {
                for(u32 i = 0; i + len <= subs.Size(); i += step)
                {
                    double e = 0.0;
                    for(u32 k = 0; k < len; k++)
                        e += subs[i + k];
                    out.PushBack(e / len);
                }
            }
            double Integrated() const
            {
                TDArray<double> blocks;
                Blocks(4, 1, blocks);
                double sum = 0.0;
                u32 n      = 0;
                for(double e : blocks)
                    if(Lufs(e) > -70.0)
                    {
                        sum += e;
                        n++;
                    }
                if(n == 0)
                    return -70.0;
                double rel = Lufs(sum / n) - 10.0;
                sum = 0.0, n = 0;
                for(double e : blocks)
                    if(Lufs(e) > -70.0 && Lufs(e) > rel)
                    {
                        sum += e;
                        n++;
                    }
                return n ? Lufs(sum / n) : -70.0;
            }
            double Range() const
            {
                TDArray<double> blocks;
                Blocks(30, 10, blocks);
                TDArray<double> gated;
                double sum = 0.0;
                for(double e : blocks)
                    if(Lufs(e) > -70.0)
                    {
                        gated.PushBack(e);
                        sum += e;
                    }
                if(gated.Size() < 2)
                    return 0.0;
                double rel = Lufs(sum / gated.Size()) - 20.0;
                TDArray<double> l;
                for(double e : gated)
                    if(Lufs(e) > rel)
                        l.PushBack(Lufs(e));
                if(l.Size() < 2)
                    return 0.0;
                l.Sort([](double a, double b) { return a < b; });
                auto pct = [&](double p) { return l[(u32)Maths::Min<double>(l.Size() - 1, floor(p * (l.Size() - 1) + 0.5))]; };
                return pct(0.95) - pct(0.10);
            }
        };
    }

    bool AnalyseAudioFile(const std::string& path, AudioAnalysisResult& out, float maxSeconds)
    {
        out = {};
        AudioDecoder* dec = AudioDecoder::Open(path);
        if(!dec)
            return false;
        const u32 rate = dec->GetSampleRate();
        const u32 ch   = Maths::Max(1u, dec->GetChannels());
        if(rate == 0)
        {
            delete dec;
            return false;
        }
        // skip intros: start a quarter of the way in when the track is long enough
        u64 total = dec->GetTotalFrames();
        u64 want  = (u64)(maxSeconds * rate);
        if(total > want * 4 / 3)
            dec->Seek((total - want) / 4);

        const u32 factor = Maths::Max(1u, rate / TargetRate);
        const float sr   = (float)rate / factor;
        LoudnessMeter meter;
        meter.Init(rate, ch);
        float peak = 0.0f;
        TDArray<float> mono;
        mono.Reserve((size_t)(maxSeconds * sr) + 16);
        TDArray<i16> buf;
        buf.Resize(4096 * ch);
        double sumSq = 0.0;
        u64 frames   = 0;
        float acc    = 0.0f;
        u32 accN     = 0;
        while(frames < want && !s_Stop.load())
        {
            u32 n = dec->Read(buf.Data(), 4096);
            if(n == 0)
                break;
            for(u32 i = 0; i < n; i++)
            {
                float fr[2] = { buf[i * ch] / 32768.0f, buf[i * ch + (ch > 1 ? 1 : 0)] / 32768.0f };
                meter.Push(fr);
                peak = Maths::Max(peak, Maths::Max(fabsf(fr[0]), fabsf(fr[1])));
                float v = 0.0f;
                for(u32 c = 0; c < ch; c++)
                    v += buf[i * ch + c];
                v /= 32768.0f * ch;
                sumSq += (double)v * v;
                acc += v;
                if(++accN == factor)
                {
                    mono.PushBack(acc / factor);
                    acc  = 0.0f;
                    accN = 0;
                }
            }
            frames += n;
        }
        delete dec;
        if(s_Stop.load() || mono.Size() < FFTSize * 8)
            return false;

        out.duration = (float)frames / rate;
        out.loudness = (float)meter.Integrated();
        out.peakDb   = peak > 0.0f ? 20.0f * log10f(peak) : -96.0f;
        float rmsDb  = 10.0f * log10f((float)(sumSq / Maths::Max<u64>(1, frames)) + 1e-12f);
        out.energy   = Maths::Clamp((rmsDb + 32.0f) / 24.0f, 0.0f, 1.0f);

        // ---- spectra: onset envelope, chroma, centroid
        const u32 nFrames = (u32)((mono.Size() - FFTSize) / Hop);
        TDArray<float> onset;
        onset.Resize(nFrames, 0.0f);
        float window[FFTSize], re[FFTSize], im[FFTSize], prev[FFTSize / 2] = {};
        for(u32 i = 0; i < FFTSize; i++)
            window[i] = 0.5f - 0.5f * cosf(2.0f * 3.14159265f * i / (FFTSize - 1));
        float chroma[12] = {};
        double centroidSum = 0.0, centroidW = 0.0;
        const float binHz = sr / FFTSize;
        // precomputed pitch class per bin, -1 outside ~55Hz..2kHz
        i32 pc[FFTSize / 2];
        for(u32 b = 0; b < FFTSize / 2; b++)
        {
            float f = b * binHz;
            if(f < 55.0f || f > 2000.0f)
                pc[b] = -1;
            else
            {
                i32 midi = (i32)lroundf(69.0f + 12.0f * log2f(f / 440.0f));
                pc[b]    = ((midi % 12) + 12) % 12;
            }
        }

        for(u32 fi = 0; fi < nFrames && !s_Stop.load(); fi++)
        {
            const float* src = mono.Data() + (size_t)fi * Hop;
            for(u32 i = 0; i < FFTSize; i++)
            {
                re[i] = src[i] * window[i];
                im[i] = 0.0f;
            }
            FFT(re, im, FFTSize);
            float flux = 0.0f, magSum = 0.0f, weighted = 0.0f;
            for(u32 b = 1; b < FFTSize / 2; b++)
            {
                float mag = sqrtf(re[b] * re[b] + im[b] * im[b]);
                float lm  = logf(1.0f + 100.0f * mag);
                float d   = lm - prev[b];
                if(d > 0.0f)
                    flux += d;
                prev[b] = lm;
                if(pc[b] >= 0)
                    chroma[pc[b]] += lm;
                magSum += mag;
                weighted += mag * b * binHz;
            }
            onset[fi] = flux;
            if(magSum > 1e-6f)
            {
                centroidSum += weighted / magSum * magSum;
                centroidW += magSum;
            }
        }
        if(s_Stop.load())
            return false;
        out.brightness = centroidW > 0.0 ? Maths::Clamp((float)(centroidSum / centroidW) / 3500.0f, 0.0f, 1.0f) : 0.0f;

        // ---- tempo: autocorrelation of the (detrended) onset envelope
        float mean = 0.0f;
        for(u32 i = 0; i < nFrames; i++)
            mean += onset[i];
        mean /= Maths::Max(1u, nFrames);
        for(u32 i = 0; i < nFrames; i++)
            onset[i] = Maths::Max(0.0f, onset[i] - mean);
        const float fps = sr / Hop;
        u32 minLag      = (u32)(fps * 60.0f / 200.0f);
        u32 maxLag      = Maths::Min(nFrames / 2, (u32)(fps * 60.0f / 55.0f));
        float zero = 0.0f;
        for(u32 i = 0; i < nFrames; i++)
            zero += onset[i] * onset[i];
        float best = 0.0f, bestLag = 0.0f;
        TDArray<float> ac;
        ac.Resize(maxLag + 2, 0.0f);
        for(u32 lag = minLag; lag <= maxLag; lag++)
        {
            float s = 0.0f;
            for(u32 i = 0; i + lag < nFrames; i++)
                s += onset[i] * onset[i + lag];
            ac[lag] = s / (nFrames - lag);
        }
        for(u32 lag = minLag + 1; lag < maxLag; lag++)
        {
            float bpm = 60.0f * fps / lag;
            // prefer ~120 BPM (log-normal prior), and reward lags whose double also lines up
            float prior = expf(-0.5f * powf(log2f(bpm / 120.0f) / 0.9f, 2.0f));
            float s     = ac[lag] + (lag * 2 <= maxLag ? 0.5f * ac[lag * 2] : 0.0f);
            s *= prior;
            if(ac[lag] >= ac[lag - 1] && ac[lag] >= ac[lag + 1] && s > best)
            {
                best    = s;
                // parabolic peak refinement
                float a = ac[lag - 1], b = ac[lag], c = ac[lag + 1];
                float d = a - 2.0f * b + c;
                bestLag = (float)lag + (fabsf(d) > 1e-9f ? 0.5f * (a - c) / d : 0.0f);
            }
        }
        float strength = zero > 0.0f ? (best / (zero / nFrames)) : 0.0f;
        out.beatStrength = Maths::Clamp(strength, 0.0f, 1.0f);
        if(bestLag > 0.0f && out.beatStrength > 0.05f)
            out.bpm = roundf(60.0f * fps / bestLag * 10.0f) / 10.0f;

        // ---- key
        float bestKey = -2.0f, second = -2.0f;
        for(u32 k = 0; k < 12; k++)
        {
            for(u32 m = 0; m < 2; m++)
            {
                float r = Correlate(chroma, m ? s_Minor : s_Major, k);
                if(r > bestKey)
                {
                    second    = bestKey;
                    bestKey   = r;
                    out.key   = (i32)k;
                    out.minor = m == 1;
                }
                else if(r > second)
                    second = r;
            }
        }
        out.keyConfidence = Maths::Clamp((bestKey - second) * 4.0f + bestKey * 0.5f, 0.0f, 1.0f);
        out.ok = true;
        return true;
    }

    // ---------------------------------------------------------------- quality

    namespace
    {
        // 4x oversampling by windowed sinc, 12 taps per phase: enough for a true-peak estimate
        struct TruePeak
        {
            static constexpr int Taps = 12;
            float coef[3][Taps];
            float hist[2][Taps] = {};
            int pos             = 0;
            float peak          = 0.0f;
            TruePeak()
            {
                for(int ph = 1; ph <= 3; ph++)
                {
                    float frac = ph / 4.0f;
                    float sum  = 0.0f;
                    for(int k = 0; k < Taps; k++)
                    {
                        float x = (float)(k - Taps / 2 + 1) - frac; // distance from the new point to tap k
                        float sinc = fabsf(x) < 1e-6f ? 1.0f : sinf(3.14159265f * x) / (3.14159265f * x);
                        float w    = 0.5f + 0.5f * cosf(3.14159265f * x / (Taps / 2 + 0.5f));
                        coef[ph - 1][k] = sinc * w;
                        sum += sinc * w;
                    }
                    for(int k = 0; k < Taps; k++)
                        coef[ph - 1][k] /= sum;
                }
            }
            inline void Push(const float* frame, u32 channels)
            {
                pos = (pos + 1) % Taps;
                for(u32 c = 0; c < channels; c++)
                {
                    hist[c][pos] = frame[c];
                    peak         = Maths::Max(peak, fabsf(frame[c]));
                    for(int ph = 0; ph < 3; ph++)
                    {
                        float v = 0.0f;
                        for(int k = 0; k < Taps; k++)
                            v += coef[ph][k] * hist[c][(pos + 1 + k) % Taps];
                        peak = Maths::Max(peak, fabsf(v));
                    }
                }
            }
        };

        // perceptual-ish "magma" ramp for the spectrogram
        void Colour(float t, u8* out)
        {
            static const float stops[6][3] = { { 0, 0, 0.02f }, { 0.18f, 0.05f, 0.35f }, { 0.55f, 0.1f, 0.5f }, { 0.9f, 0.3f, 0.3f }, { 0.99f, 0.65f, 0.25f }, { 1, 0.98f, 0.8f } };
            t       = Maths::Clamp(t, 0.0f, 1.0f) * 5.0f;
            int i   = Maths::Min(4, (int)t);
            float f = t - i;
            for(int c = 0; c < 3; c++)
                out[c] = (u8)(255.0f * (stops[i][c] + (stops[i + 1][c] - stops[i][c]) * f));
            out[3] = 255;
        }
    }

    bool AnalyseAudioQuality(const std::string& path, AudioQualityResult& out, u32 specW, u32 specH, float maxSeconds)
    {
        out = {};
        AudioDecoder* dec = AudioDecoder::OpenPrecise(path);
        if(!dec)
            return false;
        const u32 rate = dec->GetSampleRate();
        const u32 ch   = Maths::Clamp(dec->GetChannels(), 1u, 2u);
        if(rate == 0)
        {
            delete dec;
            return false;
        }
        out.codec       = dec->GetCodec();
        out.lossless    = dec->IsLossless();
        out.floatSource = dec->IsFloatSource();
        out.sourceBits  = dec->GetSourceBits();
        out.sampleRate  = rate;
        out.channels    = dec->GetChannels();

        const u64 total = dec->GetTotalFrames();
        const u64 want  = (u64)(maxSeconds * rate);
        const u64 limit = total > 0 ? Maths::Min(total, want) : want;

        LoudnessMeter meter;
        meter.Init(rate, ch);
        TruePeak tp;

        // DR: 3s blocks per channel
        const u32 drLen = rate * 3;
        TDArray<float> drRms[2], drPeak[2];
        double drSum[2] = {}, drP[2] = {};
        u32 drN = 0;

        double sum2[2] = {}, sumDC[2] = {}, sumLR = 0.0, sumM = 0.0, sumS = 0.0;
        u32 clipRun[2] = {};
        u32 bitsOr     = 0;
        const bool measureBits = out.lossless && !out.floatSource && out.sourceBits > 0 && out.sourceBits <= 24;
        const double bitScale  = measureBits ? (double)(1u << (out.sourceBits - 1)) : 0.0;

        // spectra: a ring of mono samples, FFTs at evenly spaced points
        constexpr u32 Ring = 8192;
        TDArray<float> ring;
        ring.Resize(Ring, 0.0f);
        constexpr u32 AvgN = 4096;
        TDArray<double> avg;
        avg.Resize(AvgN / 2, 0.0);
        u32 avgCount = 0;
        const u32 avgWindows = 200;
        u64 nextAvg = limit > 0 ? Maths::Max<u64>(1, limit / (avgWindows + 1)) : 0;
        const u32 SpecN = 2048;
        TDArray<float> specCols; // specW * (SpecN/2) dB values
        if(specW > 0 && specH > 0 && limit > 0)
            specCols.Resize((size_t)specW * (SpecN / 2), -140.0f);
        u32 nextCol = 0;
        auto colEnd = [&](u32 c) { return (u64)(((double)c + 0.5) / specW * limit) + SpecN / 2; };

        TDArray<float> re, im, win;
        re.Resize(AvgN), im.Resize(AvgN), win.Resize(AvgN);
        auto fftAt = [&](u64 frameEnd, u32 n, auto&& use)
        {
            for(u32 i = 0; i < n; i++)
            {
                float w = 0.5f - 0.5f * cosf(2.0f * 3.14159265f * i / (n - 1));
                re[i]   = ring[(u32)((frameEnd - n + i) % Ring)] * w;
                im[i]   = 0.0f;
            }
            FFT(re.Data(), im.Data(), n);
            use();
        };

        TDArray<float> buf;
        buf.Resize(4096 * dec->GetChannels());
        u64 frame = 0;
        while(frame < limit && !s_Stop.load())
        {
            u32 n = dec->ReadFloat(buf.Data(), (u32)Maths::Min<u64>(4096, limit - frame));
            if(n == 0)
                break;
            const u32 stride = dec->GetChannels();
            for(u32 i = 0; i < n; i++)
            {
                float fr[2];
                fr[0] = buf[i * stride];
                fr[1] = ch > 1 ? buf[i * stride + 1] : fr[0];
                meter.Push(fr);
                tp.Push(fr, ch);
                for(u32 c = 0; c < ch; c++)
                {
                    float v = fr[c];
                    sum2[c] += (double)v * v;
                    sumDC[c] += v;
                    drSum[c] += (double)v * v;
                    drP[c] = Maths::Max(drP[c], (double)fabsf(v));
                    if(fabsf(v) >= 0.9999f)
                    {
                        out.clippedSamples++;
                        if(++clipRun[c] == 3)
                            out.clipEvents++;
                    }
                    else
                        clipRun[c] = 0;
                    if(measureBits)
                    {
                        long q = lround((double)v * bitScale);
                        bitsOr |= (u32)(q < 0 ? -q : q);
                    }
                }
                if(ch > 1)
                {
                    sumLR += (double)fr[0] * fr[1];
                    double m = 0.5 * (fr[0] + fr[1]), sd = 0.5 * (fr[0] - fr[1]);
                    sumM += m * m;
                    sumS += sd * sd;
                }
                if(++drN == drLen)
                {
                    for(u32 c = 0; c < ch; c++)
                    {
                        drRms[c].PushBack((float)sqrt(2.0 * drSum[c] / drLen));
                        drPeak[c].PushBack((float)drP[c]);
                        drSum[c] = drP[c] = 0.0;
                    }
                    drN = 0;
                }
                ring[(u32)(frame % Ring)] = 0.5f * (fr[0] + fr[1]);
                frame++;
                if(nextAvg > 0 && frame == nextAvg)
                {
                    if(frame >= AvgN)
                    {
                        fftAt(frame, AvgN, [&]()
                              {
                            for(u32 b = 0; b < AvgN / 2; b++)
                                avg[b] += (double)re[b] * re[b] + (double)im[b] * im[b]; });
                        avgCount++;
                    }
                    nextAvg += Maths::Max<u64>(1, limit / (avgWindows + 1));
                }
                while(!specCols.Empty() && nextCol < specW && frame == colEnd(nextCol))
                {
                    if(frame >= SpecN)
                        fftAt(frame, SpecN, [&]()
                              {
                            float* col = specCols.Data() + (size_t)nextCol * (SpecN / 2);
                            for(u32 b = 0; b < SpecN / 2; b++)
                            {
                                float mag = sqrtf(re[b] * re[b] + im[b] * im[b]) / (SpecN / 4);
                                col[b]    = 20.0f * log10f(mag + 1e-9f);
                            } });
                    nextCol++;
                }
            }
        }
        delete dec;
        if(frame == 0 || s_Stop.load())
            return false;

        out.duration       = (float)frame / rate;
        out.integratedLufs = (float)meter.Integrated();
        out.loudnessRange  = (float)meter.Range();
        double peak = 0.0, rms2 = 0.0;
        for(u32 c = 0; c < ch; c++)
        {
            rms2 += sum2[c] / frame;
            out.dcOffset = Maths::Max(out.dcOffset, (float)fabs(sumDC[c] / frame));
        }
        rms2 /= ch;
        peak                = tp.peak;
        out.truePeakDb      = peak > 0.0 ? 20.0f * (float)log10(peak) : -96.0f;
        float samplePeak    = 0.0f;
        for(u32 c = 0; c < ch; c++)
            for(float p : drPeak[c])
                samplePeak = Maths::Max(samplePeak, p);
        // blocks shorter than 3s never landed in drPeak: fold the remainder in
        for(u32 c = 0; c < ch; c++)
            samplePeak = Maths::Max(samplePeak, (float)drP[c]);
        out.samplePeakDb = samplePeak > 0.0f ? 20.0f * log10f(samplePeak) : -96.0f;
        out.crestDb      = rms2 > 0.0 ? out.samplePeakDb - 10.0f * (float)log10(rms2) : 0.0f;
        if(ch > 1)
        {
            double l2 = sum2[0], r2 = sum2[1];
            out.correlation = (l2 > 0.0 && r2 > 0.0) ? (float)(sumLR / sqrt(l2 * r2)) : 1.0f;
            out.sideDb      = (sumS > 0.0 && sumM > 0.0) ? 10.0f * (float)log10(sumS / sumM) : -96.0f;
        }

        // DR: top 20% loudest 3s blocks against the second-highest peak, averaged over channels
        float drTotal = 0.0f;
        u32 drCh      = 0;
        for(u32 c = 0; c < ch; c++)
        {
            TDArray<float> r = drRms[c], pk = drPeak[c];
            if(r.Size() < 2)
                continue;
            r.Sort([](float a, float b) { return a > b; });
            pk.Sort([](float a, float b) { return a > b; });
            u32 top = Maths::Max(1u, (u32)(r.Size() / 5));
            double s2 = 0.0;
            for(u32 i = 0; i < top; i++)
                s2 += (double)r[i] * r[i];
            double rms = sqrt(s2 / top);
            if(rms > 0.0 && pk[1] > 0.0f)
            {
                drTotal += 20.0f * (float)log10(pk[1] / rms);
                drCh++;
            }
        }
        out.dynamicRange = drCh ? drTotal / drCh : 0.0f;

        if(measureBits)
        {
            u32 tz = 0;
            while(bitsOr && !(bitsOr & 1))
            {
                bitsOr >>= 1;
                tz++;
            }
            out.effectiveBits = bitsOr ? out.sourceBits - tz : 0;
        }

        // cutoff: highest frequency clearly above the spectrum's top-end floor
        if(avgCount > 0)
        {
            const u32 bins = AvgN / 2;
            TDArray<float> db;
            db.Resize(bins);
            for(u32 b = 0; b < bins; b++)
                db[b] = 10.0f * log10f((float)(avg[b] / avgCount) + 1e-20f);
            TDArray<float> sm;
            sm.Resize(bins);
            for(u32 b = 0; b < bins; b++)
            {
                float s = 0.0f;
                int n   = 0;
                for(int k = -4; k <= 4; k++)
                {
                    int j = (int)b + k;
                    if(j >= 0 && j < (int)bins)
                    {
                        s += db[j];
                        n++;
                    }
                }
                sm[b] = s / n;
            }
            float peakDb = -300.0f;
            for(u32 b = 1; b < bins; b++)
                peakDb = Maths::Max(peakDb, sm[b]);
            float floorDb = 1e9f;
            for(u32 b = bins - bins / 50; b < bins - 2; b++)
                floorDb = Maths::Min(floorDb, sm[b]);
            float threshold = Maths::Max(floorDb + 15.0f, peakDb - 90.0f);
            u32 cut         = 0;
            for(u32 b = bins - 3; b > 0; b--)
                if(sm[b] > threshold)
                {
                    cut = b;
                    break;
                }
            // no drop-off anywhere: content runs right up to Nyquist
            if(floorDb + 15.0f >= peakDb && peakDb > -150.0f)
                cut = bins;
            out.cutoffHz = (float)cut * rate / AvgN;
        }

        // spectrogram: frequency rows (row 0 = Nyquist), -120..0 dB
        if(!specCols.Empty())
        {
            out.specWidth  = specW;
            out.specHeight = specH;
            out.spectrogram.Resize((size_t)specW * specH * 4);
            const u32 bins = SpecN / 2;
            for(u32 x = 0; x < specW; x++)
            {
                const float* col = specCols.Data() + (size_t)x * bins;
                for(u32 y = 0; y < specH; y++)
                {
                    float f0 = (float)(specH - 1 - y) / specH * bins, f1 = (float)(specH - y) / specH * bins;
                    float v  = -140.0f;
                    for(u32 b = (u32)f0; b <= Maths::Min(bins - 1, (u32)f1); b++)
                        v = Maths::Max(v, col[b]);
                    Colour((v + 120.0f) / 120.0f, out.spectrogram.Data() + ((size_t)y * specW + x) * 4);
                }
            }
        }
        out.ok = true;
        return true;
    }

    // ---------------------------------------------------------------- worker

    namespace
    {
        struct Job
        {
            u32 id = 0;
            std::string path;
            bool quality = false;
            u32 w = 0, h = 0;
        };

        struct Worker
        {
            Mutex Lock;
            TDArray<Job> Queue;
            std::unordered_map<u32, AudioAnalysisResult> Done;
            std::unordered_map<u32, AudioQualityResult> DoneQuality;
            std::thread Thread;
            std::atomic<bool> Running { false };
            u32 NextId = 1;
            u32 Busy   = 0;

            Worker() { MutexInit(&Lock); }
            ~Worker() { AudioAnalyser::Shutdown(); }

            void Run()
            {
                while(Running.load())
                {
                    Job job;
                    {
                        ScopedMutex l(&Lock);
                        if(!Queue.Empty())
                        {
                            job = Queue[0];
                            for(u32 i = 1; i < Queue.Size(); i++)
                                Queue[i - 1] = Queue[i];
                            Queue.PopBack();
                            Busy = job.id;
                        }
                    }
                    if(job.id == 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
                        continue;
                    }
                    if(job.quality)
                    {
                        AudioQualityResult q;
                        AnalyseAudioQuality(job.path, q, job.w, job.h);
                        ScopedMutex l(&Lock);
                        DoneQuality[job.id] = q;
                    }
                    else
                    {
                        AudioAnalysisResult r;
                        AnalyseAudioFile(job.path, r);
                        ScopedMutex l(&Lock);
                        Done[job.id] = r;
                    }
                    ScopedMutex l(&Lock);
                    Busy = 0;
                }
            }
        };

        Worker& GetWorker()
        {
            static Worker s_Worker;
            return s_Worker;
        }
    }

    namespace AudioAnalyser
    {
        static u32 Push(const std::string& path, bool quality, u32 sw, u32 sh)
        {
            Worker& w = GetWorker();
            u32 id;
            {
                ScopedMutex l(&w.Lock);
                id = w.NextId++;
                Job j;
                j.id = id, j.path = path, j.quality = quality, j.w = sw, j.h = sh;
                w.Queue.PushBack(j);
            }
            if(!w.Running.load())
            {
                s_Stop    = false;
                w.Running = true;
                w.Thread  = std::thread([&w]() { w.Run(); });
            }
            return id;
        }

        u32 Queue(const std::string& path) { return Push(path, false, 0, 0); }
        u32 QueueQuality(const std::string& path, u32 sw, u32 sh) { return Push(path, true, sw, sh); }

        bool PollQuality(u32 id, AudioQualityResult& result)
        {
            Worker& w = GetWorker();
            ScopedMutex l(&w.Lock);
            auto it = w.DoneQuality.find(id);
            if(it == w.DoneQuality.end())
                return false;
            result = std::move(it->second);
            w.DoneQuality.erase(it);
            return true;
        }

        bool Poll(u32 id, AudioAnalysisResult& result)
        {
            Worker& w = GetWorker();
            ScopedMutex l(&w.Lock);
            auto it = w.Done.find(id);
            if(it == w.Done.end())
                return false;
            result = it->second;
            w.Done.erase(it);
            return true;
        }

        u32 Pending()
        {
            Worker& w = GetWorker();
            ScopedMutex l(&w.Lock);
            return (u32)w.Queue.Size() + (w.Busy ? 1u : 0u);
        }

        void ClearQueue()
        {
            Worker& w = GetWorker();
            ScopedMutex l(&w.Lock);
            w.Queue.Clear();
        }

        void Shutdown()
        {
            Worker& w = GetWorker();
            if(!w.Running.load())
                return;
            s_Stop    = true;
            w.Running = false;
            if(w.Thread.joinable())
                w.Thread.join();
            s_Stop = false;
        }
    }
}
