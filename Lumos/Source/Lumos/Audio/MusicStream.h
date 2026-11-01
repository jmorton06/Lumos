#pragma once
#include "Core/Core.h"
#include "Core/Mutex.h"
#include <string>

namespace Lumos
{
    class AudioDecoder;
    class TimeStretchDecoder;
    class VarispeedDecoder;

    // Where file streams play and how strictly. Non-default settings use the platform's native
    // backend where there is one (CoreAudio on macOS); elsewhere streams stay on OpenAL.
    struct MusicOutputConfig
    {
        std::string deviceUID; // empty = system default
        bool matchRate = false; // switch the device to each file's sample rate
        bool exclusive = false; // take the device for this app only (hog mode)
        bool native    = false; // use the native backend even with the above off
        bool matchBits = false; // exclusive only: switch the device's physical format to the file's bit depth
        bool dither    = true;  // TPDF dither when samples are reduced to the device's depth
        bool dop       = false; // exclusive only: DSD files go to the DAC as DoP instead of converted to PCM
    };

    struct MusicOutputStatus
    {
        const char* backend = "openal";
        std::string deviceName;
        u32 fileRate = 0, fileBits = 0, deviceRate = 0, deviceBits = 0;
        bool floatPath   = false; // decoded at full precision (not 16-bit)
        bool rateMatched = false; // no resampling
        bool exclusive   = false;
        bool unityGain   = false; // volume at 100%, samples untouched
        bool lossyLink   = false; // Bluetooth / AirPlay: the device re-encodes whatever it gets
        bool dithering   = false; // reduced to the device's depth with dither
        bool dop         = false; // DSD passed to the DAC as DoP
        double latency   = 0.0;   // seconds between render and speaker
        bool BitPerfect() const { return floatPath && rateMatched && unityGain && !lossyLink && (deviceBits == 0 || fileBits == 0 || deviceBits >= fileBits); }
    };

    struct MusicMeters
    {
        float peak[2] = { 0, 0 }; // linear 0..1+
        float rms[2]  = { 0, 0 };
    };

    // Streams a long audio file (music, ambience) instead of decoding it all up front.
    // Stereo is kept, no 3D positioning. Buffers are refilled on a background thread,
    // so main-thread hitches don't cause dropouts.
    class LUMOS_EXPORT MusicStream
    {
    public:
        static SharedPtr<MusicStream> Create(const std::string& path);
        // Streams any decoder (a synth, a generator). Takes ownership of it.
        static SharedPtr<MusicStream> CreateFromDecoder(const std::string& name, AudioDecoder* decoder, u32 bufferFrames = 4096, u32 bufferCount = 6);
        // Stops the refill thread and frees all AL objects. Call before the audio device goes away.
        static void ShutdownAll();

        // Applies to streams created afterwards
        static void SetOutputConfig(const MusicOutputConfig& config);
        static const MusicOutputConfig& GetOutputConfig();

        MusicStream();
        virtual ~MusicStream();

        virtual void Play()  = 0;
        virtual void Pause() = 0;
        virtual void Stop()  = 0;
        virtual void Seek(double seconds) = 0;

        virtual bool IsPlaying() const  = 0;
        virtual bool IsPaused() const   = 0;
        virtual bool IsFinished() const = 0;

        virtual double GetPosition() const = 0;
        double GetDuration() const;

        // Gapless: the file that plays straight after this one (same sample rate / channels, else
        // false and it'll need its own stream). Queuing again replaces it until it has started.
        bool QueueNext(const std::string& path);
        void ClearNext();
        // How many queued files have started playing since the stream opened (or the last seek)
        u32 GetTrackIndex() const;

        virtual void SetVolume(float volume) = 0;
        float GetVolume() const { return m_Volume; }
        virtual void SetPitch(float pitch) = 0;
        float GetPitch() const { return m_Pitch; }
        // Playback speed with the pitch kept (0.5 - 3; files only). Position stays in source time.
        void SetSpeed(float speed);
        float GetSpeed() const;
        // tape-style: speed and pitch together (0.5 - 2, 1 = off)
        void SetVarispeed(float rate);
        float GetVarispeed() const;
        void SetLooping(bool loop) { m_Looping = loop; }
        bool GetLooping() const { return m_Looping; }

        u32 GetSampleRate() const { return m_SampleRate; }
        u32 GetChannels() const { return m_Channels; }
        const std::string& GetPath() const { return m_Path; }

        // Log-spaced magnitudes (0..1) of what's audible right now. Returns bands written.
        u32 GetSpectrum(float* out, u32 bands);
        // RMS level (0..1) around the play cursor.
        float GetLevel();
        // Linear FFT bins (0..1, WebAudio-style dB mapping + smoothing) - bin i covers
        // i * sampleRate / 2048 Hz, so 512 bins reach ~11kHz like Shadertoy's music input.
        u32 GetFFT(float* out, u32 bins);
        // Mono samples (-1..1) leading up to the play cursor, for oscilloscopes.
        u32 GetWaveform(float* out, u32 count);
        // Per-channel peak / RMS over the last ~300ms of what's audible
        MusicMeters GetMeters();

        virtual MusicOutputStatus GetOutputStatus() const { return {}; }
        // Live streams (radio): the current song, and whether it's waiting on the network
        virtual std::string GetStreamTitle() const { return {}; }
        virtual bool IsBuffering() const { return false; }

    protected:
        // Must cover the backend's decode-ahead plus the analysis window (native path buffers up to ~0.7s at 192k)
        static constexpr u32 HistorySize = 262144; // mono frames kept for the visualiser (power of 2)
        static constexpr u32 MeterBlock  = 256;    // frames per meter block
        static constexpr u32 MeterBlocks = HistorySize / MeterBlock;

        // Absolute frame at the play head, for the scope (called without the scope lock held)
        virtual u64 ScopePlayFrame() const = 0;
        virtual bool ScopeActive() const   = 0;

        // Record frames handed to the output. `first` is their absolute frame index.
        void ScopePushS16(const i16* samples, u32 frames, u32 channels, u64 first);
        void ScopePushF32(const float* samples, u32 frames, u32 channels, u64 first);
        void ScopeReset(u64 frame);


        std::string m_Path;
        double m_Duration = 0.0;
        float m_Volume    = 1.0f;
        float m_Pitch     = 1.0f;
        bool m_Looping    = false;
        u32 m_SampleRate  = 0;
        u32 m_Channels    = 0;
        TimeStretchDecoder* m_Stretch = nullptr; // owned by the backend's decoder slot
        VarispeedDecoder* m_Varispeed = nullptr; // same
        class ChainDecoder* m_Chain   = nullptr; // inside m_Stretch, files only
        bool m_PreciseDecoders        = false;   // native output opens files for full precision

        // Source frame at the play head (backends; under their own lock)
        virtual u64 PlayedSourceFrame() const { return 0; }
        // Position / clamp for a source frame: within the current file when files are chained
        u64 LocalFrame(u64 sourceFrame, u64* total) const;

    private:
        u32 GatherWindow(float* out, u32 count, bool centred);
        bool ComputeMagnitudes();
        template <typename T>
        void ScopePush(const T* samples, u32 frames, u32 channels, u64 first, float scale);

        mutable Mutex m_ScopeLock;
        float* m_History     = nullptr; // mono ring indexed by absolute frame
        u64 m_HistoryStart   = 0;       // oldest valid frame
        u64 m_HistoryEnd     = 0;       // one past the newest
        struct MeterBlockData
        {
            float peak[2];
            float sumSq[2];
        };
        MeterBlockData* m_Meters = nullptr; // ring indexed by absolute frame / MeterBlock
        float m_Mag[1024]       = {};
        float m_FFTSmooth[1024] = {};
    };
}
