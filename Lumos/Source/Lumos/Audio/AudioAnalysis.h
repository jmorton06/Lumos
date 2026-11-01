#pragma once
#include "Core/Core.h"
#include "Core/DataStructures/TDArray.h"
#include <string>

namespace Lumos
{
    struct AudioAnalysisResult
    {
        bool ok          = false;
        float bpm        = 0.0f;  // 0 when there's no steady beat
        float beatStrength = 0.0f; // 0..1, how clear the pulse is
        i32 key          = -1;    // pitch class 0 = C .. 11 = B
        bool minor       = false;
        float keyConfidence = 0.0f; // 0..1
        float energy     = 0.0f;  // 0..1 loudness
        float brightness = 0.0f;  // 0..1 spectral centroid
        float duration   = 0.0f;  // seconds analysed
        float loudness   = -70.0f; // integrated LUFS over the analysed span (approximate: not the whole file)
        float peakDb     = -96.0f; // sample peak, dBFS
    };

    // Deep "audiophile" check of a whole file (up to maxSeconds).
    struct AudioQualityResult
    {
        bool ok = false;
        std::string codec;        // mp3, aac, vorbis, flac, alac, pcm, other
        bool lossless      = false;
        bool floatSource   = false;
        u32 sourceBits     = 0;   // declared bit depth (0 for lossy)
        u32 effectiveBits  = 0;   // bits actually used (0 when not measurable)
        u32 sampleRate     = 0;
        u32 channels       = 0;
        float duration     = 0.0f; // seconds analysed
        float integratedLufs = -70.0f; // EBU R128
        float loudnessRange  = 0.0f;   // LU
        float samplePeakDb   = -96.0f;
        float truePeakDb     = -96.0f; // 4x oversampled
        float crestDb        = 0.0f;
        float dynamicRange   = 0.0f;   // DR meter style (dB)
        u32 clipEvents       = 0;      // runs of 3+ samples at full scale
        u64 clippedSamples   = 0;
        float dcOffset       = 0.0f;   // largest channel mean (-1..1)
        float correlation    = 1.0f;   // L/R, 1 = mono
        float sideDb         = -96.0f; // side vs mid energy
        float cutoffHz       = 0.0f;   // highest frequency with real content
        u32 specWidth = 0, specHeight = 0;
        TDArray<u8> spectrogram;       // RGBA8, row 0 = Nyquist
    };

    // specWidth/Height > 0 also renders a spectrogram image
    LUMOS_EXPORT bool AnalyseAudioQuality(const std::string& path, AudioQualityResult& out, u32 specWidth = 0, u32 specHeight = 0,
                                          float maxSeconds = 1800.0f);

    // Tempo / key / energy from the audio itself. Decodes up to maxSeconds from the middle-ish of the file.
    LUMOS_EXPORT bool AnalyseAudioFile(const std::string& path, AudioAnalysisResult& out, float maxSeconds = 90.0f);

    // One background worker, so a library scan doesn't take over the CPU.
    namespace AudioAnalyser
    {
        LUMOS_EXPORT u32 Queue(const std::string& path); // job id (never 0)
        // true once finished (result.ok false if the file couldn't be analysed)
        LUMOS_EXPORT bool Poll(u32 id, AudioAnalysisResult& result);
        LUMOS_EXPORT u32 QueueQuality(const std::string& path, u32 specWidth = 0, u32 specHeight = 0);
        LUMOS_EXPORT bool PollQuality(u32 id, AudioQualityResult& result);
        LUMOS_EXPORT u32 Pending();
        LUMOS_EXPORT void ClearQueue();
        LUMOS_EXPORT void Shutdown();
    }
}
