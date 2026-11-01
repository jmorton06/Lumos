#include "Precompiled.h"
#include "Maths/MathsUtilities.h"
#include "AudioDecoder.h"
#include "DSDDecoder.h"
#include "RemoteAudio.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/StringUtilities.h"
#include "Core/DataStructures/TDArray.h"

#include <dr_libs/dr_mp3.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

#include <cstdio>
#include <cmath>

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
#define LUMOS_APPLE_AUDIO
#include <AudioToolbox/AudioToolbox.h>
#endif

namespace Lumos
{
    namespace
    {
        std::string ResolvePath(const std::string& path)
        {
            if(path.size() < 2 || path[0] != '/' || path[1] != '/')
                return path;

            std::string out   = path;
            ArenaTemp scratch = ScratchBegin(0, 0);
            String8 resolved  = {};
            if(FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8StdS(path), &resolved) && resolved.size)
                out = std::string((const char*)resolved.str, resolved.size);
            ScratchEnd(scratch);
            return out;
        }

        class Mp3Decoder : public AudioDecoder
        {
        public:
            bool Init(const std::string& path)
            {
                if(!drmp3_init_file(&m_MP3, path.c_str(), nullptr))
                    return false;
                m_Open       = true;
                m_SampleRate = m_MP3.sampleRate;
                m_Channels   = m_MP3.channels;
                // Xing/Info header gives this for free; otherwise we have to scan
                m_TotalFrames = m_MP3.totalPCMFrameCount != DRMP3_UINT64_MAX ? m_MP3.totalPCMFrameCount : drmp3_get_pcm_frame_count(&m_MP3);
                m_Codec       = "mp3";
                return m_Channels > 0 && m_SampleRate > 0;
            }

            ~Mp3Decoder() override
            {
                if(m_Open)
                    drmp3_uninit(&m_MP3);
                delete[] m_SeekPoints;
            }

            u32 Read(i16* out, u32 frames) override
            {
                return (u32)drmp3_read_pcm_frames_s16(&m_MP3, frames, out);
            }

            u32 ReadFloat(float* out, u32 frames) override
            {
                return (u32)drmp3_read_pcm_frames_f32(&m_MP3, frames, out);
            }

            bool Seek(u64 frame) override
            {
                // Without a seek table dr_mp3 decodes from the start - build one on first seek
                if(!m_SeekPoints)
                {
                    drmp3_uint32 count = 1024;
                    m_SeekPoints       = new drmp3_seek_point[count];
                    if(drmp3_calculate_seek_points(&m_MP3, &count, m_SeekPoints))
                        drmp3_bind_seek_table(&m_MP3, count, m_SeekPoints);
                }
                return drmp3_seek_to_pcm_frame(&m_MP3, frame);
            }

        private:
            drmp3 m_MP3                    = {};
            drmp3_seek_point* m_SeekPoints = nullptr;
            bool m_Open                    = false;
        };

        class VorbisDecoder : public AudioDecoder
        {
        public:
            bool Init(const std::string& path)
            {
                int error = 0;
                m_Vorbis  = stb_vorbis_open_filename(path.c_str(), &error, nullptr);
                if(!m_Vorbis)
                    return false;
                stb_vorbis_info info = stb_vorbis_get_info(m_Vorbis);
                m_SampleRate         = info.sample_rate;
                m_Channels           = info.channels > 2 ? 2 : info.channels; // stb mixes down for us
                m_TotalFrames        = stb_vorbis_stream_length_in_samples(m_Vorbis);
                m_Codec              = "vorbis";
                return m_Channels > 0;
            }

            ~VorbisDecoder() override
            {
                if(m_Vorbis)
                    stb_vorbis_close(m_Vorbis);
            }

            u32 Read(i16* out, u32 frames) override
            {
                return (u32)stb_vorbis_get_samples_short_interleaved(m_Vorbis, (int)m_Channels, out, (int)(frames * m_Channels));
            }

            u32 ReadFloat(float* out, u32 frames) override
            {
                return (u32)stb_vorbis_get_samples_float_interleaved(m_Vorbis, (int)m_Channels, out, (int)(frames * m_Channels));
            }

            bool Seek(u64 frame) override
            {
                return stb_vorbis_seek(m_Vorbis, (unsigned int)frame) != 0;
            }

        private:
            stb_vorbis* m_Vorbis = nullptr;
        };

        // Plain RIFF WAV: 8/16/24/32-bit PCM and 32-bit float.
        class WavDecoder : public AudioDecoder
        {
        public:
            bool Init(const std::string& path)
            {
                m_File = fopen(path.c_str(), "rb");
                if(!m_File)
                    return false;

                u8 header[12];
                if(fread(header, 1, 12, m_File) != 12 || memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0)
                    return false;

                u16 format = 0;
                bool gotFmt = false;
                u8 chunk[8];
                while(fread(chunk, 1, 8, m_File) == 8)
                {
                    u32 size = chunk[4] | (chunk[5] << 8) | (chunk[6] << 16) | ((u32)chunk[7] << 24);
                    if(memcmp(chunk, "fmt ", 4) == 0)
                    {
                        u8 fmt[40] = {};
                        u32 toRead = size < 40 ? size : 40;
                        if(fread(fmt, 1, toRead, m_File) != toRead)
                            return false;
                        if(size > toRead)
                            fseek(m_File, size - toRead, SEEK_CUR);
                        format          = fmt[0] | (fmt[1] << 8);
                        m_FileChannels  = fmt[2] | (fmt[3] << 8);
                        m_SampleRate    = fmt[4] | (fmt[5] << 8) | (fmt[6] << 16) | ((u32)fmt[7] << 24);
                        m_BitsPerSample = fmt[14] | (fmt[15] << 8);
                        if(format == 0xFFFE && toRead >= 26)
                            format = fmt[24] | (fmt[25] << 8); // WAVE_FORMAT_EXTENSIBLE sub-format
                        gotFmt = true;
                    }
                    else if(memcmp(chunk, "data", 4) == 0)
                    {
                        m_DataOffset = ftell(m_File);
                        m_DataSize   = size;
                        break;
                    }
                    else
                        fseek(m_File, size + (size & 1), SEEK_CUR);
                }

                if(!gotFmt || m_DataOffset == 0 || m_FileChannels == 0)
                    return false;

                m_IsFloat = format == 3;
                if(!(format == 1 || (m_IsFloat && m_BitsPerSample == 32)))
                    return false;
                if(m_BitsPerSample != 8 && m_BitsPerSample != 16 && m_BitsPerSample != 24 && m_BitsPerSample != 32)
                    return false;

                m_Channels    = m_FileChannels > 2 ? 2 : m_FileChannels;
                m_FrameBytes  = m_FileChannels * (m_BitsPerSample / 8);
                m_TotalFrames = m_DataSize / m_FrameBytes;
                m_Codec       = "pcm";
                m_Lossless    = true;
                m_SourceBits  = m_BitsPerSample;
                m_FloatSource = m_IsFloat;
                return true;
            }

            ~WavDecoder() override
            {
                if(m_File)
                    fclose(m_File);
            }

            u32 Read(i16* out, u32 frames) override
            {
                u64 left = m_TotalFrames - m_Cursor;
                if(frames > left)
                    frames = (u32)left;
                if(frames == 0)
                    return 0;

                u8 buf[4096];
                u32 done              = 0;
                const u32 maxPerChunk = sizeof(buf) / m_FrameBytes;
                while(done < frames)
                {
                    u32 n = Maths::Min(frames - done, maxPerChunk);
                    n     = (u32)fread(buf, m_FrameBytes, n, m_File);
                    if(n == 0)
                        break;
                    for(u32 f = 0; f < n; f++)
                        for(u32 c = 0; c < m_Channels; c++)
                            out[(done + f) * m_Channels + c] = SampleAt(buf + f * m_FrameBytes + c * (m_BitsPerSample / 8));
                    done += n;
                }
                m_Cursor += done;
                return done;
            }

            bool Seek(u64 frame) override
            {
                if(frame > m_TotalFrames)
                    frame = m_TotalFrames;
                m_Cursor = frame;
                return fseek(m_File, (long)(m_DataOffset + frame * m_FrameBytes), SEEK_SET) == 0;
            }

            u32 ReadFloat(float* out, u32 frames) override
            {
                u64 left = m_TotalFrames - m_Cursor;
                if(frames > left)
                    frames = (u32)left;
                u8 buf[4096];
                u32 done              = 0;
                const u32 maxPerChunk = sizeof(buf) / m_FrameBytes;
                while(done < frames)
                {
                    u32 n = Maths::Min(frames - done, maxPerChunk);
                    n     = (u32)fread(buf, m_FrameBytes, n, m_File);
                    if(n == 0)
                        break;
                    for(u32 f = 0; f < n; f++)
                        for(u32 c = 0; c < m_Channels; c++)
                            out[(done + f) * m_Channels + c] = FloatAt(buf + f * m_FrameBytes + c * (m_BitsPerSample / 8));
                    done += n;
                }
                m_Cursor += done;
                return done;
            }

        private:
            float FloatAt(const u8* p) const
            {
                switch(m_BitsPerSample)
                {
                case 8:
                    return ((int)p[0] - 128) / 128.0f;
                case 16:
                    return (i16)(p[0] | (p[1] << 8)) / 32768.0f;
                case 24:
                {
                    i32 v = (i32)((u32)p[0] << 8 | (u32)p[1] << 16 | (u32)p[2] << 24) >> 8;
                    return v / 8388608.0f;
                }
                case 32:
                    if(m_IsFloat)
                    {
                        float f;
                        memcpy(&f, p, 4);
                        return f;
                    }
                    {
                        i32 v;
                        memcpy(&v, p, 4);
                        return (float)((double)v / 2147483648.0);
                    }
                }
                return 0.0f;
            }

            i16 SampleAt(const u8* p) const
            {
                switch(m_BitsPerSample)
                {
                case 8:
                    return (i16)(((int)p[0] - 128) << 8);
                case 16:
                    return (i16)(p[0] | (p[1] << 8));
                case 24:
                    return (i16)(p[1] | (p[2] << 8));
                case 32:
                    if(m_IsFloat)
                    {
                        float f;
                        memcpy(&f, p, 4);
                        f = f < -1.0f ? -1.0f : (f > 1.0f ? 1.0f : f);
                        return (i16)(f * 32767.0f);
                    }
                    return (i16)(p[2] | (p[3] << 8));
                }
                return 0;
            }

            FILE* m_File        = nullptr;
            long m_DataOffset   = 0;
            u32 m_DataSize      = 0;
            u32 m_FileChannels  = 0;
            u32 m_BitsPerSample = 0;
            u32 m_FrameBytes    = 0;
            u64 m_Cursor        = 0;
            bool m_IsFloat      = false;
        };


        // AIFF / AIFC: 8/16/24/32-bit PCM (big-endian, or 'sowt' little-endian) and 32/64-bit float
        class AiffDecoder : public AudioDecoder
        {
        public:
            bool Init(const std::string& path)
            {
                m_File = fopen(path.c_str(), "rb");
                if(!m_File)
                    return false;
                u8 header[12];
                if(fread(header, 1, 12, m_File) != 12 || memcmp(header, "FORM", 4) != 0)
                    return false;
                const bool aifc = memcmp(header + 8, "AIFC", 4) == 0;
                if(!aifc && memcmp(header + 8, "AIFF", 4) != 0)
                    return false;

                bool gotComm = false;
                u8 chunk[8];
                while(fread(chunk, 1, 8, m_File) == 8)
                {
                    const u32 size = BE32(chunk + 4);
                    if(memcmp(chunk, "COMM", 4) == 0)
                    {
                        u8 comm[26] = {};
                        const u32 toRead = size < 26 ? size : 26;
                        if(fread(comm, 1, toRead, m_File) != toRead || toRead < 18)
                            return false;
                        m_FileChannels  = (u32)((comm[0] << 8) | comm[1]);
                        m_TotalFrames   = BE32(comm + 2);
                        m_BitsPerSample = (u32)((comm[6] << 8) | comm[7]);
                        m_SampleRate    = (u32)(Extended(comm + 8) + 0.5);
                        if(aifc && toRead >= 22)
                        {
                            if(!memcmp(comm + 18, "sowt", 4))
                                m_Little = true;
                            else if(!memcmp(comm + 18, "fl32", 4) || !memcmp(comm + 18, "FL32", 4))
                                m_Float = true, m_BitsPerSample = 32;
                            else if(!memcmp(comm + 18, "fl64", 4) || !memcmp(comm + 18, "FL64", 4))
                                m_Float = true, m_BitsPerSample = 64;
                            else if(memcmp(comm + 18, "NONE", 4) && memcmp(comm + 18, "twos", 4))
                                return false; // compressed (ulaw, ima4...): leave it to the OS decoder
                        }
                        const u32 padded = size + (size & 1);
                        if(padded > toRead)
                            fseek(m_File, (long)(padded - toRead), SEEK_CUR);
                        gotComm = true;
                    }
                    else if(memcmp(chunk, "SSND", 4) == 0)
                    {
                        u8 ss[8];
                        if(fread(ss, 1, 8, m_File) != 8)
                            return false;
                        m_DataOffset = ftell(m_File) + (long)BE32(ss);
                        break;
                    }
                    else
                        fseek(m_File, (long)(size + (size & 1)), SEEK_CUR);
                }
                if(!gotComm || !m_DataOffset || !m_FileChannels || !m_SampleRate)
                    return false;
                if(m_BitsPerSample < 8 || (m_BitsPerSample > 32 && !(m_Float && m_BitsPerSample == 64)))
                    return false;
                m_Bytes      = (m_BitsPerSample + 7) / 8;
                m_FrameBytes = m_FileChannels * m_Bytes;
                m_Channels   = m_FileChannels > 2 ? 2 : m_FileChannels;
                fseek(m_File, m_DataOffset, SEEK_SET);
                m_Codec       = "pcm";
                m_Lossless    = true;
                m_SourceBits  = m_BitsPerSample;
                m_FloatSource = m_Float;
                return true;
            }

            ~AiffDecoder() override
            {
                if(m_File)
                    fclose(m_File);
            }

            u32 ReadFloat(float* out, u32 frames) override
            {
                u64 left = m_TotalFrames - m_Cursor;
                if(frames > left)
                    frames = (u32)left;
                u8 buf[8192];
                u32 done              = 0;
                const u32 maxPerChunk = sizeof(buf) / m_FrameBytes;
                while(done < frames)
                {
                    u32 n = Maths::Min(frames - done, maxPerChunk);
                    n     = (u32)fread(buf, m_FrameBytes, n, m_File);
                    if(n == 0)
                        break;
                    for(u32 f = 0; f < n; f++)
                        for(u32 c = 0; c < m_Channels; c++)
                            out[(done + f) * m_Channels + c] = Sample(buf + f * m_FrameBytes + c * m_Bytes);
                    done += n;
                }
                m_Cursor += done;
                return done;
            }

            u32 Read(i16* out, u32 frames) override
            {
                m_Tmp.Resize((size_t)frames * m_Channels);
                u32 n = ReadFloat(m_Tmp.Data(), frames);
                for(u32 i = 0; i < n * m_Channels; i++)
                {
                    float v = m_Tmp[i] * 32767.0f;
                    out[i]  = (i16)(v > 32767.0f ? 32767.0f : (v < -32768.0f ? -32768.0f : v));
                }
                return n;
            }

            bool Seek(u64 frame) override
            {
                if(frame > m_TotalFrames)
                    frame = m_TotalFrames;
                m_Cursor = frame;
                return fseek(m_File, (long)(m_DataOffset + frame * m_FrameBytes), SEEK_SET) == 0;
            }

        private:
            static u32 BE32(const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }

            // 80-bit IEEE extended (the COMM sample rate)
            static double Extended(const u8* p)
            {
                const int expo = ((p[0] & 0x7F) << 8) | p[1];
                u64 mant       = 0;
                for(int i = 0; i < 8; i++)
                    mant = (mant << 8) | p[2 + i];
                if(expo == 0 && mant == 0)
                    return 0.0;
                const double v = std::ldexp((double)mant, expo - 16383 - 63);
                return (p[0] & 0x80) ? -v : v;
            }

            float Sample(const u8* p) const
            {
                // bytes most significant first
                u8 b[8];
                for(u32 i = 0; i < m_Bytes; i++)
                    b[i] = m_Little ? p[m_Bytes - 1 - i] : p[i];
                if(m_Float)
                {
                    if(m_Bytes == 8)
                    {
                        u64 bits = 0;
                        for(int i = 0; i < 8; i++)
                            bits = (bits << 8) | b[i];
                        double d;
                        memcpy(&d, &bits, 8);
                        return (float)d;
                    }
                    u32 bits = ((u32)b[0] << 24) | ((u32)b[1] << 16) | ((u32)b[2] << 8) | b[3];
                    float f;
                    memcpy(&f, &bits, 4);
                    return f;
                }
                i32 v = 0;
                for(u32 i = 0; i < m_Bytes; i++)
                    v = (i32)((u32)v << 8 | b[i]);
                const u32 shift = 32 - m_Bytes * 8;
                v               = (i32)((u32)v << shift) >> shift; // sign-extend
                return (float)((double)v / (double)(1ull << (m_Bytes * 8 - 1)));
            }

            FILE* m_File        = nullptr;
            long m_DataOffset   = 0;
            u32 m_FileChannels  = 0;
            u32 m_BitsPerSample = 0;
            u32 m_Bytes         = 0;
            u32 m_FrameBytes    = 0;
            u64 m_Cursor        = 0;
            bool m_Little       = false;
            bool m_Float        = false;
            TDArray<float> m_Tmp;
        };

#ifdef LUMOS_APPLE_AUDIO
        // AAC/ALAC/M4A/AIFF/CAF via ExtAudioFile - the OS does the decoding.
        class AppleAudioDecoder : public AudioDecoder
        {
        public:
            bool Init(const std::string& path, bool precise = false)
            {
                m_Precise    = precise;
                CFURLRef url = CFURLCreateFromFileSystemRepresentation(nullptr, (const UInt8*)path.c_str(), (CFIndex)path.size(), false);
                if(!url)
                    return false;
                OSStatus err = ExtAudioFileOpenURL(url, &m_File);
                CFRelease(url);
                if(err != noErr || !m_File)
                {
                    m_File = nullptr;
                    return false;
                }
                return Setup();
            }

            // Reads come from `src` (a file still downloading); the extension tells the OS what to expect
            bool InitSource(const AudioByteSource& src, const std::string& ext)
            {
                m_Precise = true;
                m_Source  = src;
                AudioFileTypeID hint = 0;
                if(ext == "mp3")
                    hint = kAudioFileMP3Type;
                else if(ext == "m4a" || ext == "m4b" || ext == "mp4" || ext == "alac")
                    hint = kAudioFileM4AType;
                else if(ext == "aac")
                    hint = kAudioFileAAC_ADTSType;
                else if(ext == "flac")
                    hint = kAudioFileFLACType;
                else if(ext == "wav")
                    hint = kAudioFileWAVEType;
                else if(ext == "aif" || ext == "aiff" || ext == "aifc")
                    hint = kAudioFileAIFFType;
                else if(ext == "caf")
                    hint = kAudioFileCAFType;
                if(AudioFileOpenWithCallbacks(this, SourceRead, nullptr, SourceSize, nullptr, hint, &m_AudioFile) != noErr)
                {
                    m_AudioFile = nullptr;
                    return false;
                }
                if(ExtAudioFileWrapAudioFileID(m_AudioFile, false, &m_File) != noErr || !m_File)
                {
                    m_File = nullptr;
                    return false;
                }
                return Setup();
            }

            static OSStatus SourceRead(void* user, SInt64 position, UInt32 count, void* buffer, UInt32* actual)
            {
                auto* self = (AppleAudioDecoder*)user;
                u64 size   = self->m_Source.size(self->m_Source.user);
                size_t n   = self->m_Source.read(self->m_Source.user, (u64)position, buffer, count);
                *actual    = (UInt32)n;
                // short only at the end of the file; anything else is the network giving up
                return (n == count || (u64)position + n >= size) ? noErr : kAudioFileUnspecifiedError;
            }

            static SInt64 SourceSize(void* user)
            {
                auto* self = (AppleAudioDecoder*)user;
                return (SInt64)self->m_Source.size(self->m_Source.user);
            }

            bool Setup()
            {
                AudioStreamBasicDescription fileFormat = {};
                UInt32 size                            = sizeof(fileFormat);
                if(ExtAudioFileGetProperty(m_File, kExtAudioFileProperty_FileDataFormat, &size, &fileFormat) != noErr)
                    return false;

                m_SampleRate = (u32)fileFormat.mSampleRate;
                m_Channels   = fileFormat.mChannelsPerFrame > 2 ? 2 : fileFormat.mChannelsPerFrame;
                if(m_SampleRate == 0 || m_Channels == 0)
                    return false;

                switch(fileFormat.mFormatID)
                {
                case kAudioFormatAppleLossless: m_Codec = "alac"; m_Lossless = true; break;
                case kAudioFormatFLAC: m_Codec = "flac"; m_Lossless = true; break;
                case kAudioFormatLinearPCM:
                    m_Codec       = "pcm";
                    m_Lossless    = true;
                    m_SourceBits  = fileFormat.mBitsPerChannel;
                    m_FloatSource = (fileFormat.mFormatFlags & kAudioFormatFlagIsFloat) != 0;
                    break;
                case kAudioFormatMPEG4AAC:
                case kAudioFormatMPEG4AAC_HE:
                case kAudioFormatMPEG4AAC_HE_V2: m_Codec = "aac"; break;
                case kAudioFormatMPEGLayer3: m_Codec = "mp3"; break;
                default: m_Codec = "other"; break;
                }
                if(m_Lossless && m_SourceBits == 0)
                {
                    // ALAC / FLAC keep the source depth in the file
                    AudioFileID af = nullptr;
                    size           = sizeof(af);
                    if(ExtAudioFileGetProperty(m_File, kExtAudioFileProperty_AudioFile, &size, &af) == noErr && af)
                    {
                        SInt32 bits = 0;
                        size        = sizeof(bits);
                        if(AudioFileGetProperty(af, kAudioFilePropertySourceBitDepth, &size, &bits) == noErr)
                        {
                            m_FloatSource = bits < 0;
                            m_SourceBits  = (u32)(bits < 0 ? -bits : bits);
                        }
                    }
                }

                AudioStreamBasicDescription client = {};
                client.mSampleRate                 = fileFormat.mSampleRate;
                client.mFormatID                   = kAudioFormatLinearPCM;
                client.mChannelsPerFrame           = m_Channels;
                client.mFormatFlags                = m_Precise ? (kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked)
                                                               : (kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked);
                client.mBitsPerChannel             = m_Precise ? 32 : 16;
                client.mBytesPerFrame              = (m_Precise ? 4 : 2) * m_Channels;
                client.mFramesPerPacket            = 1;
                client.mBytesPerPacket             = client.mBytesPerFrame;
                if(ExtAudioFileSetProperty(m_File, kExtAudioFileProperty_ClientDataFormat, sizeof(client), &client) != noErr)
                    return false;

                SInt64 frames = 0;
                size          = sizeof(frames);
                if(ExtAudioFileGetProperty(m_File, kExtAudioFileProperty_FileLengthFrames, &size, &frames) == noErr && frames > 0)
                    m_TotalFrames = (u64)frames;
                return true;
            }

            ~AppleAudioDecoder() override
            {
                if(m_File)
                    ExtAudioFileDispose(m_File);
                if(m_AudioFile)
                    AudioFileClose(m_AudioFile);
            }

            u32 Read(i16* out, u32 frames) override
            {
                if(m_Precise)
                {
                    // analysis instances decode to float; convert for callers that want s16
                    TDArray<float> tmp;
                    tmp.Resize((size_t)frames * m_Channels);
                    u32 n = ReadFloat(tmp.Data(), frames);
                    for(u32 i = 0; i < n * m_Channels; i++)
                    {
                        float v = tmp[i] < -1.0f ? -1.0f : (tmp[i] > 1.0f ? 1.0f : tmp[i]);
                        out[i]  = (i16)(v * 32767.0f);
                    }
                    return n;
                }
                AudioBufferList list;
                list.mNumberBuffers              = 1;
                list.mBuffers[0].mNumberChannels = m_Channels;
                list.mBuffers[0].mDataByteSize   = frames * m_Channels * sizeof(i16);
                list.mBuffers[0].mData           = out;
                UInt32 count                     = frames;
                if(ExtAudioFileRead(m_File, &count, &list) != noErr)
                    return 0;
                return count;
            }

            u32 ReadFloat(float* out, u32 frames) override
            {
                if(!m_Precise)
                    return AudioDecoder::ReadFloat(out, frames);
                AudioBufferList list;
                list.mNumberBuffers              = 1;
                list.mBuffers[0].mNumberChannels = m_Channels;
                list.mBuffers[0].mDataByteSize   = frames * m_Channels * sizeof(float);
                list.mBuffers[0].mData           = out;
                UInt32 count                     = frames;
                if(ExtAudioFileRead(m_File, &count, &list) != noErr)
                    return 0;
                return count;
            }

            bool Seek(u64 frame) override
            {
                return ExtAudioFileSeek(m_File, (SInt64)frame) == noErr;
            }

        private:
            ExtAudioFileRef m_File   = nullptr;
            AudioFileID m_AudioFile  = nullptr; // only when reading through m_Source
            AudioByteSource m_Source = {};
            bool m_Precise           = false;
        };
#endif

        template <typename T>
        AudioDecoder* TryOpen(const std::string& path)
        {
            T* dec = new T();
            if(dec->Init(path))
                return dec;
            delete dec;
            return nullptr;
        }
    }

    u32 AudioDecoder::ReadFloat(float* out, u32 frames)
    {
        TDArray<i16> tmp;
        tmp.Resize((size_t)frames * m_Channels);
        u32 n = Read(tmp.Data(), frames);
        for(u32 i = 0; i < n * m_Channels; i++)
            out[i] = tmp[i] / 32768.0f;
        return n;
    }

    AudioDecoder* AudioDecoder::OpenPrecise(const std::string& pathIn)
    {
        if(RemoteAudio::IsUrl(pathIn))
            return RemoteAudio::Open(pathIn, true);
#ifdef LUMOS_APPLE_AUDIO
        std::string path = ResolvePath(pathIn);
        std::string ext  = StringUtilities::ToLower(StringUtilities::GetFilePathExtension(path));
        if(ext != "mp3" && ext != "ogg" && ext != "wav" && ext != "aif" && ext != "aiff" && ext != "aifc" && !DSDDecoder::IsDSDExtension(ext))
        {
            AppleAudioDecoder* dec = new AppleAudioDecoder();
            if(dec->Init(path, true))
                return dec;
            delete dec;
        }
#endif
        return Open(pathIn);
    }

    AudioDecoder* AudioDecoder::OpenSource(const AudioByteSource& src, const std::string& extIn)
    {
#ifdef LUMOS_APPLE_AUDIO
        AppleAudioDecoder* dec = new AppleAudioDecoder();
        if(dec->InitSource(src, StringUtilities::ToLower(extIn)))
            return dec;
        delete dec;
#endif
        // TODO: dr_mp3 / dr_wav over callbacks for the other platforms
        return nullptr;
    }

    bool AudioDecoder::IsSupportedExtension(const std::string& extIn)
    {
        std::string ext = StringUtilities::ToLower(extIn);
        if(ext == "mp3" || ext == "ogg" || ext == "wav" || ext == "aif" || ext == "aiff" || ext == "aifc" || DSDDecoder::IsDSDExtension(ext))
            return true;
#ifdef LUMOS_APPLE_AUDIO
        if(ext == "m4a" || ext == "m4b" || ext == "aac" || ext == "mp4" || ext == "alac" || ext == "aif" || ext == "aiff" || ext == "caf" || ext == "flac")
            return true;
#endif
        return false;
    }

    AudioDecoder* AudioDecoder::Open(const std::string& pathIn)
    {
        if(RemoteAudio::IsUrl(pathIn))
            return RemoteAudio::Open(pathIn, false);
        std::string path = ResolvePath(pathIn);
        std::string ext  = StringUtilities::ToLower(StringUtilities::GetFilePathExtension(path));

        AudioDecoder* dec = nullptr;
        if(ext == "mp3")
            dec = TryOpen<Mp3Decoder>(path);
        else if(ext == "ogg")
            dec = TryOpen<VorbisDecoder>(path);
        else if(ext == "wav")
            dec = TryOpen<WavDecoder>(path);

        else if(ext == "aif" || ext == "aiff" || ext == "aifc")
            dec = TryOpen<AiffDecoder>(path);
        else if(DSDDecoder::IsDSDExtension(ext))
            return DSDDecoder::Open(path, false); // nothing else reads it

#ifdef LUMOS_APPLE_AUDIO
        // Also the fallback for anything the portable decoders refused. Float out: the stream,
        // EQ and speed all work in float, so 24-bit files keep their detail.
        if(!dec)
        {
            AppleAudioDecoder* apple = new AppleAudioDecoder();
            if(apple->Init(path, true))
                dec = apple;
            else
                delete apple;
        }
#endif

        if(!dec)
            LWARN("AudioDecoder: can't decode '%s'", path.c_str());
        return dec;
    }
}
