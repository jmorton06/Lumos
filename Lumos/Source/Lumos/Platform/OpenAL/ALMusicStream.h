#pragma once
#include "Audio/MusicStream.h"
#include "Core/Mutex.h"

#include <AL/al.h>

namespace Lumos
{
    class AudioDecoder;

    class ALMusicStream : public MusicStream
    {
    public:
        static constexpr u32 MaxBuffers = 8;

        // Takes ownership of the decoder. Small buffers = low latency (live synths), big = fewer wakeups.
        ALMusicStream(const std::string& path, AudioDecoder* decoder, u32 bufferFrames = 4096, u32 bufferCount = 6);
        ~ALMusicStream() override;

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
        void SetPitch(float pitch) override;

        MusicOutputStatus GetOutputStatus() const override;
        std::string GetStreamTitle() const override;
        bool IsBuffering() const override;

        // Background thread tick. Refills processed buffers, restarts after underruns.
        void Update();
        void Release();

        static void ShutdownAll();
        // any stream playing (not paused / finished)
        static bool AnyPlaying();

    protected:
        u64 ScopePlayFrame() const override;
        bool ScopeActive() const override;

    private:
        bool FillBuffer(ALuint buffer); // false at end of stream
        void Prime();                   // stop, unqueue everything, refill from the decoder's cursor
        u64 PlayFrameLocked() const;    // absolute (monotonic) frame at the play head

        mutable Mutex m_Lock;
        AudioDecoder* m_Decoder = nullptr;
        ALuint m_Source         = 0;
        ALuint m_Buffers[MaxBuffers] = {};
        u32 m_BufferCount  = 6;
        u32 m_BufferFrames = 4096;
        ALenum m_Format         = 0;

        // Frame counts of queued buffers, oldest first
        u32 m_QueuedFrames[MaxBuffers] = {};
        u32 m_QueueHead  = 0;
        u32 m_QueueCount = 0;

        // Absolute frame counters: keep counting across loops, so file position = frame % length.
        // A seek resets them to the target frame.
        u64 m_PlayedFrames  = 0; // frames fully played out of the queue
        u64 m_DecodedFrames = 0; // frames handed to AL

        bool m_WantPlaying = false;
        bool m_Paused      = false;
        bool m_EndOfFile   = false;
        bool m_Released    = false;

        i16* m_Scratch          = nullptr; // 16-bit upload when AL has no float buffers
        float* m_FloatScratch   = nullptr;
        bool m_Float            = false;
        u32 m_DitherSeed        = 0x9E3779B9u;
    };
}
