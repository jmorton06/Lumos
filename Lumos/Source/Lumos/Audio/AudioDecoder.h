#pragma once
#include "Core/Core.h"
#include <string>

namespace Lumos
{
    // Bytes from somewhere other than a local file (a download in progress). read may block.
    struct AudioByteSource
    {
        void* user = nullptr;
        size_t (*read)(void* user, u64 offset, void* dst, size_t bytes) = nullptr;
        u64 (*size)(void* user) = nullptr;
    };

    // Pull-style decoder for streaming long files. Output is interleaved s16.
    class AudioDecoder
    {
    public:
        // Picks a decoder from the file extension. Physical paths or //VFS paths; http(s) URLs go
        // through RemoteAudio (streamed, cached as they play).
        // mp3/ogg/wav everywhere; m4a/aac/alac/aiff/caf via the OS on Apple platforms.
        static AudioDecoder* Open(const std::string& path);
        // Like Open, but asks for full-precision samples (analysis): ReadFloat keeps 24/32-bit detail
        static AudioDecoder* OpenPrecise(const std::string& path);
        static bool IsSupportedExtension(const std::string& ext);
        // Decodes from `src`; `ext` says what kind of file it is. Apple platforms only for now.
        static AudioDecoder* OpenSource(const AudioByteSource& src, const std::string& ext);

        virtual ~AudioDecoder() = default;

        // Returns frames written (0 at end of stream).
        virtual u32 Read(i16* out, u32 frames) = 0;
        virtual bool Seek(u64 frame)           = 0;
        // Interleaved -1..1. Decoders with more than 16 bits of source detail override this.
        virtual u32 ReadFloat(float* out, u32 frames);
        // Live streams: the current song (ICY StreamTitle), and whether it's waiting on the network
        virtual std::string GetStreamTitle() const { return {}; }
        virtual bool IsBuffering() const { return false; }
        // DSD packed as DoP: the samples are data for the DAC and must reach it bit for bit
        virtual bool IsDoP() const { return false; }

        // What the file itself is: codec name ("mp3", "flac", "alac", "aac", "pcm", ...), lossless or not,
        // and its bit depth (0 = not meaningful, e.g. lossy; 32 for float sources)
        const std::string& GetCodec() const { return m_Codec; }
        bool IsLossless() const { return m_Lossless; }
        u32 GetSourceBits() const { return m_SourceBits; }
        bool IsFloatSource() const { return m_FloatSource; }

        u32 GetSampleRate() const { return m_SampleRate; }
        u32 GetChannels() const { return m_Channels; }
        u64 GetTotalFrames() const { return m_TotalFrames; } // 0 if unknown
        double GetDuration() const { return m_SampleRate ? (double)m_TotalFrames / m_SampleRate : 0.0; }
        // Streams count output frames; a decoder that changes speed maps them back to source frames
        virtual u64 SourceFrame(u64 outputFrame) const { return outputFrame; }

    protected:
        u32 m_SampleRate  = 0;
        u32 m_Channels    = 0;
        u64 m_TotalFrames = 0;
        std::string m_Codec;
        bool m_Lossless    = false;
        bool m_FloatSource = false;
        u32 m_SourceBits   = 0;
    };
}
