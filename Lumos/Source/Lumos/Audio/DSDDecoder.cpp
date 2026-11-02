#include "Precompiled.h"
#include "DSDDecoder.h"
#include "Utilities/StringUtilities.h"
#include "Maths/MathsUtilities.h"
#include "Core/OS/FileSystem.h"
#include <cmath>
#include <cstring>

namespace Lumos
{
    namespace
    {
        u32 LE32(const u8* p) { return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); }
        u64 LE64(const u8* p) { return (u64)LE32(p) | ((u64)LE32(p + 4) << 32); }
        u32 BE32(const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }
        u64 BE64(const u8* p) { return ((u64)BE32(p) << 32) | BE32(p + 4); }
        u16 BE16(const u8* p) { return (u16)((p[0] << 8) | p[1]); }

        u8 Reverse(u8 b)
        {
            b = (u8)((b & 0xF0) >> 4 | (b & 0x0F) << 4);
            b = (u8)((b & 0xCC) >> 2 | (b & 0x33) << 2);
            return (u8)((b & 0xAA) >> 1 | (b & 0x55) << 1);
        }

        constexpr u8 kSilence = 0x69; // DSD idle pattern
    }

    bool DSDDecoder::IsDSDExtension(const std::string& extIn)
    {
        std::string ext = StringUtilities::ToLower(extIn);
        return ext == "dsf" || ext == "dff";
    }

    DSDDecoder* DSDDecoder::Open(const std::string& pathIn, bool dop)
    {
        std::string path = pathIn;
        if(path.size() > 1 && path[0] == '/' && path[1] == '/')
        {
            ArenaTemp scratch = ScratchBegin(0, 0);
            String8 resolved  = {};
            if(FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8StdS(pathIn), &resolved) && resolved.size)
                path = std::string((const char*)resolved.str, resolved.size);
            ScratchEnd(scratch);
        }
        DSDDecoder* d = new DSDDecoder();
        d->m_File     = fopen(path.c_str(), "rb");
        if(d->m_File)
        {
            std::string ext = StringUtilities::ToLower(StringUtilities::GetFilePathExtension(path));
            d->m_IsDSF      = ext == "dsf";
            d->m_DoP        = dop;
            if(d->m_IsDSF ? d->InitDSF() : d->InitDFF())
            {
                d->BuildFilter();
                return d;
            }
        }
        delete d;
        return nullptr;
    }

    DSDDecoder::~DSDDecoder()
    {
        if(m_File)
            fclose(m_File);
    }

    bool DSDDecoder::InitDSF()
    {
        u8 h[92];
        if(fread(h, 1, 92, m_File) != 92 || memcmp(h, "DSD ", 4) || memcmp(h + 28, "fmt ", 4) || memcmp(h + 80, "data", 4))
            return false;
        const u8* f         = h + 40; // fmt body
        const u32 formatID  = LE32(f + 4);
        m_FileChannels      = LE32(f + 12);
        m_DSDRate           = LE32(f + 16);
        const u32 bits      = LE32(f + 20);
        const u64 samples   = LE64(f + 24);
        m_BlockSize         = LE32(f + 32);
        if(formatID != 0 || m_FileChannels == 0 || m_FileChannels > 6 || m_BlockSize == 0 || (bits != 1 && bits != 8))
            return false;
        m_LSBFirst   = bits == 1;
        m_DataOffset = 92;
        m_DataBytes  = samples / 8;
        return true;
    }

    bool DSDDecoder::InitDFF()
    {
        u8 h[16];
        if(fread(h, 1, 16, m_File) != 16 || memcmp(h, "FRM8", 4) || memcmp(h + 12, "DSD ", 4))
            return false;
        bool compressed = false;
        u8 c[12];
        while(fread(c, 1, 12, m_File) == 12)
        {
            const u64 size  = BE64(c + 4);
            const long body = ftell(m_File);
            if(!memcmp(c, "PROP", 4))
            {
                u8 snd[4];
                if(fread(snd, 1, 4, m_File) != 4 || memcmp(snd, "SND ", 4))
                    return false;
                u64 used = 4;
                while(used + 12 <= size)
                {
                    u8 s[12];
                    if(fread(s, 1, 12, m_File) != 12)
                        return false;
                    const u64 ssize = BE64(s + 4);
                    u8 buf[8]       = {};
                    const u32 want  = (u32)Maths::Min<u64>(ssize, 8);
                    if(fread(buf, 1, want, m_File) != want)
                        return false;
                    if(!memcmp(s, "FS  ", 4))
                        m_DSDRate = BE32(buf);
                    else if(!memcmp(s, "CHNL", 4))
                        m_FileChannels = BE16(buf);
                    else if(!memcmp(s, "CMPR", 4))
                        compressed = memcmp(buf, "DSD ", 4) != 0;
                    const u64 padded = ssize + (ssize & 1);
                    fseek(m_File, (long)(padded - want), SEEK_CUR);
                    used += 12 + padded;
                }
            }
            else if(!memcmp(c, "DSD ", 4))
            {
                if(compressed || !m_FileChannels || !m_DSDRate)
                    return false; // DST
                m_DataOffset = (u64)body;
                m_DataBytes  = size / m_FileChannels;
                return true;
            }
            fseek(m_File, body + (long)(size + (size & 1)), SEEK_SET);
        }
        return false;
    }

    void DSDDecoder::BuildFilter()
    {
        m_Channels = m_FileChannels > 2 ? 2 : m_FileChannels;
        m_Codec    = "dsd";
        m_Lossless = true;
        m_SourceBits = 1;
        if(m_DoP)
        {
            // 16 DSD bits per channel per 24-bit word: DSD64 -> 176.4 kHz
            m_SampleRate  = m_DSDRate / 16;
            m_TotalFrames = m_DataBytes / 2;
            m_Pending.Resize(2 * m_Channels);
            fseek(m_File, (long)m_DataOffset, SEEK_SET);
            return;
        }

        // 88.2 kHz (or 96 kHz for the 48k family) whatever the DSD rate
        const u32 outRate = (m_DSDRate % 44100 == 0) ? 88200 : 96000;
        m_Decim           = Maths::Max(8u, (m_DSDRate / outRate) & ~7u);
        m_SampleRate      = m_DSDRate / m_Decim;
        m_TotalFrames     = m_DataBytes * 8 / m_Decim;

        // windowed-sinc low pass at 24 kHz, Blackman: ~30 kHz transition at every DSD rate
        m_Taps          = 2 * m_Decim;      // bytes
        const u32 bits  = m_Taps * 8;
        const double fc = 24000.0 / m_DSDRate;
        TDArray<double> h;
        h.Resize(bits);
        double sum = 0.0;
        for(u32 i = 0; i < bits; i++)
        {
            const double m   = i - (bits - 1) * 0.5;
            const double sn  = m == 0.0 ? 2.0 * fc : std::sin(2.0 * Maths::M_PI * fc * m) / (Maths::M_PI * m);
            const double w   = 0.42 - 0.5 * std::cos(2.0 * Maths::M_PI * i / (bits - 1)) + 0.08 * std::cos(4.0 * Maths::M_PI * i / (bits - 1));
            h[i]             = sn * w;
            sum += h[i];
        }
        // unity at DC: 0 dB DSD (50% modulation) lands at half scale, leaving headroom for the
        // louder-than-0 dB peaks DSD allows
        const double gain = 1.0 / sum;
        m_Table.Resize((size_t)m_Taps * 256);
        for(u32 k = 0; k < m_Taps; k++)
            for(u32 b = 0; b < 256; b++)
            {
                double acc = 0.0;
                for(u32 j = 0; j < 8; j++) // bit j of the byte k bytes back is 8k + j samples old (MSB first)
                    acc += h[8 * k + j] * ((b >> j) & 1 ? 1.0 : -1.0);
                m_Table[(size_t)k * 256 + b] = (float)(acc * gain);
            }
        m_Hist.Resize((size_t)m_Taps * m_Channels, kSilence);
        m_HistPos = 0;
        m_Pending.Resize((size_t)(m_Decim / 8) * m_Channels);
        fseek(m_File, (long)m_DataOffset, SEEK_SET);
    }

    bool DSDDecoder::ReadBlock()
    {
        if(m_IsDSF)
        {
            m_Block.Resize((size_t)m_BlockSize * m_FileChannels);
            m_BlockFill = (u32)fread(m_Block.Data(), 1, m_Block.Size(), m_File) == m_Block.Size() ? m_BlockSize : 0;
        }
        else
        {
            // DFF: bytes interleaved by channel
            const u32 frames = 4096;
            m_Block.Resize((size_t)frames * m_FileChannels);
            m_BlockFill = (u32)(fread(m_Block.Data(), 1, m_Block.Size(), m_File) / m_FileChannels);
        }
        m_BlockPos = 0;
        return m_BlockFill > 0;
    }

    // count bytes of each channel into m_Pending (channel-major), MSB = earliest bit
    bool DSDDecoder::FetchBytes(u32 count)
    {
        for(u32 i = 0; i < count; i++)
        {
            if(m_BytePos >= m_DataBytes)
                return false;
            if(m_BlockPos >= m_BlockFill && !ReadBlock())
                return false;
            for(u32 c = 0; c < m_Channels; c++)
            {
                u8 b = m_IsDSF ? m_Block[(size_t)c * m_BlockSize + m_BlockPos] : m_Block[(size_t)m_BlockPos * m_FileChannels + c];
                m_Pending[(size_t)c * count + i] = m_LSBFirst ? Reverse(b) : b;
            }
            m_BlockPos++;
            m_BytePos++;
        }
        return true;
    }

    u32 DSDDecoder::ReadFloat(float* out, u32 frames)
    {
        u32 done = 0;
        if(m_DoP)
        {
            for(; done < frames; done++)
            {
                if(!FetchBytes(2))
                    break;
                // markers alternate per frame, same on every channel
                const u32 marker = ((m_BytePos / 2) & 1) ? 0xFA : 0x05;
                for(u32 c = 0; c < m_Channels; c++)
                {
                    const u32 word = (marker << 16) | ((u32)m_Pending[c * 2] << 8) | m_Pending[c * 2 + 1];
                    const i32 s    = (i32)(word << 8) >> 8; // 24-bit signed
                    out[done * m_Channels + c] = (float)s / 8388608.0f;
                }
            }
            return done;
        }

        const u32 step = m_Decim / 8;
        for(; done < frames; done++)
        {
            if(!FetchBytes(step))
                break;
            for(u32 i = 0; i < step; i++)
            {
                for(u32 c = 0; c < m_Channels; c++)
                    m_Hist[(size_t)c * m_Taps + m_HistPos] = m_Pending[(size_t)c * step + i];
                m_HistPos = (m_HistPos + 1) % m_Taps;
            }
            for(u32 c = 0; c < m_Channels; c++)
            {
                const u8* hist = m_Hist.Data() + (size_t)c * m_Taps;
                float acc      = 0.0f;
                u32 idx        = m_HistPos; // oldest
                for(u32 k = m_Taps; k-- > 0;) // k bytes back: newest is k = 0
                {
                    acc += m_Table[(size_t)k * 256 + hist[idx]];
                    idx = idx + 1 == m_Taps ? 0 : idx + 1;
                }
                out[done * m_Channels + c] = acc;
            }
        }
        return done;
    }

    u32 DSDDecoder::Read(i16* out, u32 frames)
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

    bool DSDDecoder::Seek(u64 frame)
    {
        if(frame > m_TotalFrames)
            frame = m_TotalFrames;
        u64 byte = m_DoP ? frame * 2 : frame * (m_Decim / 8);
        if(!m_DoP)
        {
            // warm the filter up from a little before the target
            u64 back = Maths::Min<u64>(byte, m_Taps);
            byte -= back;
            for(auto& b : m_Hist)
                b = kSilence;
        }
        m_BytePos = byte;
        if(m_IsDSF)
        {
            const u64 block = byte / m_BlockSize;
            fseek(m_File, (long)(m_DataOffset + block * m_BlockSize * m_FileChannels), SEEK_SET);
            ReadBlock();
            m_BlockPos = (u32)(byte % m_BlockSize);
        }
        else
        {
            fseek(m_File, (long)(m_DataOffset + byte * m_FileChannels), SEEK_SET);
            m_BlockPos = m_BlockFill = 0;
        }
        if(!m_DoP)
        {
            // feed the warm-up bytes through the history without output
            const u64 target = frame * (m_Decim / 8);
            while(m_BytePos < target)
            {
                if(!FetchBytes(1))
                    break;
                for(u32 c = 0; c < m_Channels; c++)
                    m_Hist[(size_t)c * m_Taps + m_HistPos] = m_Pending[c];
                m_HistPos = (m_HistPos + 1) % m_Taps;
            }
        }
        return true;
    }
}
