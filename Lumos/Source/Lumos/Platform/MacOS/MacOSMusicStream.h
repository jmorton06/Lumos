#pragma once
#include "Audio/MusicStream.h"
#include "Core/Mutex.h"

#include <atomic>

namespace Lumos
{
    class AudioDecoder;

    // File playback straight to a CoreAudio device (AUHAL): float samples at the file's own rate,
    // optional device rate switching and hog mode, no processing at unity volume.
    class MacOSMusicStream : public MusicStream
    {
    public:
        // Takes ownership of the decoder (should be AudioDecoder::OpenPrecise)
        MacOSMusicStream(const std::string& path, AudioDecoder* decoder, const MusicOutputConfig& config);
        ~MacOSMusicStream() override;

        bool IsValid() const { return m_Unit != nullptr; }
        // hands the decoder back (a stream that failed to open, so it can go elsewhere)
        AudioDecoder* TakeDecoder()
        {
            AudioDecoder* d = m_Decoder;
            m_Decoder       = nullptr;
            return d;
        }

        void Play() override;
        void Pause() override;
        void Stop() override;
        void Seek(double seconds) override;

        bool IsPlaying() const override;
        bool IsPaused() const override;
        bool IsFinished() const override;
        double GetPosition() const override;
        u64 PlayedSourceFrame() const override;

        void SetVolume(float volume) override;
        void SetPitch(float pitch) override { m_Pitch = pitch; } // not supported: bit-perfect path

        MusicOutputStatus GetOutputStatus() const override;

        void Update(); // feed thread: keeps the ring topped up
        void Release();

        static void ShutdownAll();
        // Hands back devices whose rate / hog mode we changed (when the config turns those off)
        static void RestoreDevices(bool rates, bool hog);

    protected:
        u64 ScopePlayFrame() const override;
        bool ScopeActive() const override;

    private:
    public:
        // Called on the CoreAudio IO thread
        void RenderInto(float* out, u32 frames);

    private:
        void Fill();
        void Restart(u64 frame); // unit must be stopped
        u64 LatencyFrames() const;

        mutable Mutex m_Lock;
        AudioDecoder* m_Decoder       = nullptr;
        void* m_Unit   = nullptr; // AudioComponentInstance
        u32 m_Device   = 0;       // AudioObjectID
        MusicOutputConfig m_Config;

        float* m_Ring       = nullptr; // stereo interleaved, indexed by absolute frame
        u32 m_RingFrames    = 0;       // power of 2
        float* m_Scratch    = nullptr;
        u32 m_ScratchFrames = 4096;
        std::atomic<u64> m_Read { 0 };  // absolute frames consumed by the device
        std::atomic<u64> m_Write { 0 }; // absolute frames decoded
        u64 m_SegmentStart = 0;         // first frame since the last seek
        std::atomic<bool> m_EndOfFile { false };
        std::atomic<float> m_Gain { 1.0f };
        float m_AppliedGain = 1.0f; // render thread only
        u32 m_DitherSeed    = 22222; // render thread only
        std::atomic<bool> m_Processed { false }; // EQ / speed are changing samples (fill thread)
        bool m_Dithering    = false;

        bool m_WantPlaying = false;
        bool m_Paused      = false;
        bool m_Running     = false;
        bool m_Released    = false;
        double m_DrainedAt = -1.0; // when the ring ran dry at end of file (device still playing its buffer)

        u32 m_DeviceRate    = 0;
        u32 m_DeviceBits    = 0;
        bool m_RateMatched  = false;
        bool m_Hogged       = false;
        bool m_DoP          = false; // samples are DoP words: no gain, no dither
        bool m_LossyLink    = false;
        double m_Latency    = 0.0;
        std::string m_DeviceName;
    };
}
