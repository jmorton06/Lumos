#include "Precompiled.h"
#include "Maths/MathsUtilities.h"
#include "AudioTags.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/StringUtilities.h"

#include <cstdio>
#include <cstring>
#include <cmath>

namespace Lumos
{
    namespace
    {
        const char* s_Genres[] = {
            "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz", "Metal",
            "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno", "Industrial",
            "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
            "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise",
            "Alternative Rock", "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic",
            "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
            "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret", "New Wave", "Psychedelic", "Rave", "Showtunes",
            "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock"
        };
        constexpr u32 s_GenreCount = sizeof(s_Genres) / sizeof(s_Genres[0]);

        struct File
        {
            FILE* f  = nullptr;
            i64 size = 0;

            bool Open(const std::string& path)
            {
                f = fopen(path.c_str(), "rb");
                if(!f)
                    return false;
                fseek(f, 0, SEEK_END);
                size = ftell(f);
                fseek(f, 0, SEEK_SET);
                return true;
            }
            ~File()
            {
                if(f)
                    fclose(f);
            }
            bool ReadAt(i64 offset, void* dst, size_t n)
            {
                if(offset < 0 || offset + (i64)n > size)
                    return false;
                fseek(f, (long)offset, SEEK_SET);
                return fread(dst, 1, n, f) == n;
            }
            bool ReadAt(i64 offset, TDArray<u8>& dst, size_t n)
            {
                dst.Resize(n);
                return n == 0 || ReadAt(offset, dst.Data(), n);
            }
        };

        u32 BE32(const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }
        u32 BE24(const u8* p) { return ((u32)p[0] << 16) | ((u32)p[1] << 8) | p[2]; }
        u16 BE16(const u8* p) { return (u16)((p[0] << 8) | p[1]); }
        u32 LE32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24); }
        u64 BE64(const u8* p) { return ((u64)BE32(p) << 32) | BE32(p + 4); }
        u32 Synchsafe(const u8* p) { return ((u32)(p[0] & 0x7f) << 21) | ((p[1] & 0x7f) << 14) | ((p[2] & 0x7f) << 7) | (p[3] & 0x7f); }

        void AppendUTF8(std::string& out, u32 cp)
        {
            if(cp < 0x80)
                out += (char)cp;
            else if(cp < 0x800)
            {
                out += (char)(0xC0 | (cp >> 6));
                out += (char)(0x80 | (cp & 0x3F));
            }
            else if(cp < 0x10000)
            {
                out += (char)(0xE0 | (cp >> 12));
                out += (char)(0x80 | ((cp >> 6) & 0x3F));
                out += (char)(0x80 | (cp & 0x3F));
            }
            else
            {
                out += (char)(0xF0 | (cp >> 18));
                out += (char)(0x80 | ((cp >> 12) & 0x3F));
                out += (char)(0x80 | ((cp >> 6) & 0x3F));
                out += (char)(0x80 | (cp & 0x3F));
            }
        }

        std::string Trim(const std::string& s)
        {
            size_t a = 0, b = s.size();
            while(a < b && (s[a] == ' ' || s[a] == '\0' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n'))
                a++;
            while(b > a && (s[b - 1] == ' ' || s[b - 1] == '\0' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n'))
                b--;
            return s.substr(a, b - a);
        }

        // Lots of taggers write UTF-8 bytes but label them Latin-1 - pass those through untouched
        bool LooksLikeUTF8(const u8* p, size_t n)
        {
            bool multi = false;
            for(size_t i = 0; i < n && p[i];)
            {
                u8 c = p[i];
                size_t len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
                if(len == 0 || i + len > n)
                    return false;
                for(size_t k = 1; k < len; k++)
                    if((p[i + k] & 0xC0) != 0x80)
                        return false;
                multi |= len > 1;
                i += len;
            }
            return multi;
        }

        std::string Latin1ToUTF8(const u8* p, size_t n)
        {
            if(LooksLikeUTF8(p, n))
            {
                size_t end = 0;
                while(end < n && p[end])
                    end++;
                return std::string((const char*)p, end);
            }
            std::string out;
            for(size_t i = 0; i < n && p[i]; i++)
                AppendUTF8(out, p[i]);
            return out;
        }

        // ID3 text: encoding byte already stripped. Stops at the first terminator (v2.4 multi-values -> first).
        std::string DecodeID3Text(u8 enc, const u8* p, size_t n, size_t* consumed = nullptr)
        {
            std::string out;
            size_t i = 0;
            if(enc == 0 || enc == 3)
            {
                size_t end = 0;
                while(end < n && p[end])
                    end++;
                out = enc == 0 ? Latin1ToUTF8(p, end) : std::string((const char*)p, end);
                i   = end < n ? end + 1 : end;
            }
            else
            {
                bool bigEndian = enc == 2;
                if(enc == 1 && n >= 2)
                {
                    if(p[0] == 0xFE && p[1] == 0xFF)
                    {
                        bigEndian = true;
                        i         = 2;
                    }
                    else if(p[0] == 0xFF && p[1] == 0xFE)
                        i = 2;
                }
                while(i + 1 < n)
                {
                    u32 c = bigEndian ? (u32)((p[i] << 8) | p[i + 1]) : (u32)(p[i] | (p[i + 1] << 8));
                    i += 2;
                    if(c == 0)
                        break;
                    if(c >= 0xD800 && c < 0xDC00 && i + 1 < n)
                    {
                        u32 lo = bigEndian ? (u32)((p[i] << 8) | p[i + 1]) : (u32)(p[i] | (p[i + 1] << 8));
                        if(lo >= 0xDC00 && lo < 0xE000)
                        {
                            c = 0x10000 + ((c - 0xD800) << 10) + (lo - 0xDC00);
                            i += 2;
                        }
                    }
                    AppendUTF8(out, c);
                }
            }
            if(consumed)
                *consumed = i;
            return Trim(out);
        }

        void ParseNumberPair(const std::string& s, u32& a, u32& b)
        {
            a = (u32)atoi(s.c_str());
            size_t slash = s.find('/');
            if(slash != std::string::npos)
                b = (u32)atoi(s.c_str() + slash + 1);
        }

        u32 ParseYear(const std::string& s)
        {
            for(size_t i = 0; i + 3 < s.size(); i++)
                if(isdigit((u8)s[i]) && isdigit((u8)s[i + 1]) && isdigit((u8)s[i + 2]) && isdigit((u8)s[i + 3]))
                    return (u32)atoi(s.substr(i, 4).c_str());
            return 0;
        }

        std::string ResolveGenre(const std::string& g)
        {
            // "(17)", "(17)Rock" or "17"
            std::string s = g;
            if(!s.empty() && s[0] == '(')
            {
                size_t close = s.find(')');
                if(close != std::string::npos)
                {
                    std::string rest = s.substr(close + 1);
                    if(!rest.empty())
                        return rest;
                    s = s.substr(1, close - 1);
                }
            }
            if(!s.empty() && s.find_first_not_of("0123456789") == std::string::npos)
            {
                u32 idx = (u32)atoi(s.c_str());
                return idx < s_GenreCount ? s_Genres[idx] : std::string();
            }
            return g;
        }

        // key lower-case: replaygain_track_gain "-6.54 dB", or Opus' r128_track_gain (Q7.8 against -23 LUFS)
        bool SetReplayGain(AudioTags& out, const std::string& key, const std::string& val)
        {
            const bool r128 = key.compare(0, 5, "r128_") == 0;
            if(!r128 && key.compare(0, 11, "replaygain_") != 0)
                return false;
            const bool track = key.find("track_gain") != std::string::npos;
            const bool album = key.find("album_gain") != std::string::npos;
            if((!track && !album) || val.empty())
                return false;
            const float db = r128 ? (float)atoi(val.c_str()) / 256.0f + 5.0f : (float)atof(val.c_str());
            if(track)
                out.TrackGain = db, out.HasTrackGain = true;
            else
                out.AlbumGain = db, out.HasAlbumGain = true;
            return true;
        }

        void SetIfEmpty(std::string& dst, const std::string& src)
        {
            if(dst.empty() && !src.empty())
                dst = src;
        }

        // ---------------------------------------------------------------- ID3v2

        // Returns total tag size (0 if none).
        u32 ParseID3v2(File& file, AudioTags& out, bool loadArtwork, i64 base = 0)
        {
            u8 hdr[10];
            if(!file.ReadAt(base, hdr, 10) || memcmp(hdr, "ID3", 3) != 0)
                return 0;

            const u8 ver   = hdr[3];
            const u8 flags = hdr[5];
            u32 size       = Synchsafe(hdr + 6);
            u32 total      = size + 10 + ((flags & 0x10) ? 10 : 0);
            if(ver < 2 || ver > 4)
                return total;

            TDArray<u8> tag;
            if(!file.ReadAt(base + 10, tag, size))
                return total;

            // whole-tag unsynchronisation (v2.3 and earlier)
            if((flags & 0x80) && ver < 4)
            {
                u32 w = 0;
                for(u32 r = 0; r < tag.Size(); r++)
                {
                    tag[w++] = tag[r];
                    if(tag[r] == 0xFF && r + 1 < tag.Size() && tag[r + 1] == 0x00)
                        r++;
                }
                tag.Resize(w);
            }

            u32 pos = 0;
            if((flags & 0x40) && ver >= 3 && tag.Size() >= 4)
                pos = ver == 3 ? BE32(&tag[0]) + 4 : Synchsafe(&tag[0]);

            const u32 idLen  = ver == 2 ? 3 : 4;
            const u32 hdrLen = ver == 2 ? 6 : 10;
            int artworkScore = -1;

            while(pos + hdrLen <= tag.Size())
            {
                const u8* fh = &tag[pos];
                if(fh[0] == 0)
                    break; // padding

                char id[5] = {};
                memcpy(id, fh, idLen);
                u32 fsize = 0;
                u16 fflags = 0;
                if(ver == 2)
                    fsize = BE24(fh + 3);
                else if(ver == 3)
                    fsize = BE32(fh + 4);
                else
                {
                    fsize = Synchsafe(fh + 4);
                    // some v2.4 writers use plain sizes - trust whichever lands on a valid next frame
                    u32 plain = BE32(fh + 4);
                    if(plain != fsize && pos + hdrLen + plain <= tag.Size())
                    {
                        u32 next = pos + hdrLen + fsize;
                        bool synchOk = next + 4 <= tag.Size() && (tag[next] == 0 || isalnum(tag[next]));
                        if(!synchOk)
                            fsize = plain;
                    }
                }
                if(ver >= 3)
                    fflags = BE16(fh + 8);

                pos += hdrLen;
                if(fsize == 0 || pos + fsize > tag.Size())
                    break;

                const u8* data = &tag[pos];
                u32 len        = fsize;
                pos += fsize;

                bool compressed = ver == 3 ? (fflags & 0x0080) : (ver == 4 ? (fflags & 0x0008) : false);
                bool encrypted  = ver == 3 ? (fflags & 0x0040) : (ver == 4 ? (fflags & 0x0004) : false);
                if(compressed || encrypted)
                    continue;

                TDArray<u8> unsynced;
                if(ver == 4)
                {
                    if(fflags & 0x0001) // data length indicator
                    {
                        if(len < 4)
                            continue;
                        data += 4;
                        len -= 4;
                    }
                    if(fflags & 0x0002) // per-frame unsync
                    {
                        unsynced.Reserve(len);
                        for(u32 r = 0; r < len; r++)
                        {
                            unsynced.PushBack(data[r]);
                            if(data[r] == 0xFF && r + 1 < len && data[r + 1] == 0)
                                r++;
                        }
                        data = unsynced.Data();
                        len  = (u32)unsynced.Size();
                    }
                }

                auto is = [&](const char* v2, const char* v34)
                { return strcmp(id, ver == 2 ? v2 : v34) == 0; };

                if(id[0] == 'T' && len > 1 && strcmp(id, "TXXX") != 0 && strcmp(id, "TXX") != 0)
                {
                    std::string text = DecodeID3Text(data[0], data + 1, len - 1);
                    if(is("TT2", "TIT2"))
                        out.Title = text;
                    else if(is("TP1", "TPE1"))
                        out.Artist = text;
                    else if(is("TAL", "TALB"))
                        out.Album = text;
                    else if(is("TP2", "TPE2"))
                        out.AlbumArtist = text;
                    else if(is("TCM", "TCOM"))
                        out.Composer = text;
                    else if(is("TST", "TSOT"))
                        out.SortTitle = text;
                    else if(is("TSP", "TSOP"))
                        out.SortArtist = text;
                    else if(is("TSA", "TSOA"))
                        out.SortAlbum = text;
                    else if(is("TS2", "TSO2"))
                        out.SortAlbumArtist = text;
                    else if(is("TCO", "TCON"))
                        out.Genre = ResolveGenre(text);
                    else if(is("TRK", "TRCK"))
                        ParseNumberPair(text, out.Track, out.TrackCount);
                    else if(is("TPA", "TPOS"))
                        ParseNumberPair(text, out.Disc, out.DiscCount);
                    else if(is("TYE", "TYER") || strcmp(id, "TDRC") == 0 || strcmp(id, "TDOR") == 0)
                    {
                        if(!out.Year)
                            out.Year = ParseYear(text);
                    }
                    else if(is("TCP", "TCMP"))
                        out.Compilation = atoi(text.c_str()) != 0;
                }
                else if((strcmp(id, "TXXX") == 0 || strcmp(id, "TXX") == 0) && len > 2)
                {
                    // description, then the value
                    size_t used      = 0;
                    std::string desc = DecodeID3Text(data[0], data + 1, len - 1, &used);
                    if(1 + used < len)
                        SetReplayGain(out, StringUtilities::ToLower(desc), DecodeID3Text(data[0], data + 1 + used, len - 1 - (u32)used));
                }
                else if(strcmp(id, "CHAP") == 0 && ver >= 3)
                {
                    // element id (latin1, null-terminated), start / end ms, start / end offsets, then sub-frames
                    u32 p = 0;
                    while(p < len && data[p])
                        p++;
                    p++;
                    if(p + 16 > len)
                        continue;
                    AudioChapter chap;
                    chap.Start = BE32(data + p) / 1000.0;
                    p += 16;
                    while(p + 10 <= len)
                    {
                        u32 sub = ver == 4 ? Synchsafe(data + p + 4) : BE32(data + p + 4);
                        if(sub == 0 || p + 10 + sub > len)
                            break;
                        if(memcmp(data + p, "TIT2", 4) == 0 && sub > 1)
                            chap.Title = DecodeID3Text(data[p + 10], data + p + 11, sub - 1);
                        p += 10 + sub;
                    }
                    out.Chapters.PushBack(chap);
                }
                else if((strcmp(id, "USLT") == 0 || strcmp(id, "ULT") == 0) && len > 4 && out.Lyrics.empty())
                {
                    // encoding, language[3], descriptor, text
                    size_t used = 0;
                    DecodeID3Text(data[0], data + 4, len - 4, &used);
                    if(4 + used < len)
                        out.Lyrics = DecodeID3Text(data[0], data + 4 + used, len - 4 - used);
                }
                else if((strcmp(id, "APIC") == 0 || strcmp(id, "PIC") == 0) && len > 4)
                {
                    out.HasArtwork = true;
                    if(!loadArtwork)
                        continue;
                    u8 enc       = data[0];
                    u32 p        = 1;
                    std::string mime;
                    if(ver == 2)
                    {
                        std::string fmt((const char*)data + 1, 3);
                        mime = StringUtilities::ToLower(fmt) == "png" ? "image/png" : "image/jpeg";
                        p    = 4;
                    }
                    else
                    {
                        while(p < len && data[p])
                            mime += (char)data[p++];
                        p++;
                    }
                    if(p >= len)
                        continue;
                    u8 picType = data[p++];
                    size_t used = 0;
                    DecodeID3Text(enc, data + p, len - p, &used);
                    // UTF-16 descriptions end in a double null that DecodeID3Text consumed; Latin1/UTF-8 in one
                    p += (u32)used;
                    if(p >= len)
                        continue;

                    int score = picType == 3 ? 2 : (picType == 0 ? 1 : 0); // front cover wins
                    if(score > artworkScore)
                    {
                        artworkScore = score;
                        out.Artwork.Resize(len - p);
                        memcpy(out.Artwork.Data(), data + p, len - p);
                        out.ArtworkMime = mime;
                    }
                }
            }
            return total;
        }

        void ParseID3v1(File& file, AudioTags& out)
        {
            u8 t[128];
            if(file.size < 128 || !file.ReadAt(file.size - 128, t, 128) || memcmp(t, "TAG", 3) != 0)
                return;
            SetIfEmpty(out.Title, Trim(Latin1ToUTF8(t + 3, 30)));
            SetIfEmpty(out.Artist, Trim(Latin1ToUTF8(t + 33, 30)));
            SetIfEmpty(out.Album, Trim(Latin1ToUTF8(t + 63, 30)));
            if(!out.Year)
                out.Year = ParseYear(std::string((const char*)t + 93, 4));
            if(!out.Track && t[125] == 0 && t[126] != 0)
                out.Track = t[126];
            if(out.Genre.empty() && t[127] < s_GenreCount)
                out.Genre = s_Genres[t[127]];
        }

        // ---------------------------------------------------------------- MPEG audio

        void ParseMPEGInfo(File& file, AudioTags& out, u32 startOffset)
        {
            static const u16 kBitrates[2][3][16] = {
                { // MPEG1: L1, L2, L3
                  { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
                  { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
                  { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 } },
                { // MPEG2/2.5
                  { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
                  { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
                  { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 } }
            };
            static const u32 kRates[3] = { 44100, 48000, 32000 };

            const u32 scan = 128 * 1024;
            TDArray<u8> buf;
            i64 avail = file.size - startOffset;
            if(avail < 4)
                return;
            if(!file.ReadAt(startOffset, buf, (size_t)Maths::Min<i64>(avail, scan)))
                return;

            for(u32 i = 0; i + 4 <= buf.Size(); i++)
            {
                const u8* h = &buf[i];
                if(h[0] != 0xFF || (h[1] & 0xE0) != 0xE0)
                    continue;
                u32 verBits = (h[1] >> 3) & 3, layerBits = (h[1] >> 1) & 3;
                u32 brIdx = h[2] >> 4, srIdx = (h[2] >> 2) & 3, pad = (h[2] >> 1) & 1;
                if(verBits == 1 || layerBits == 0 || brIdx == 0 || brIdx == 15 || srIdx == 3)
                    continue;

                bool mpeg1 = verBits == 3;
                u32 layer  = 4 - layerBits; // 1..3
                u32 rate   = kRates[srIdx] >> (mpeg1 ? 0 : (verBits == 2 ? 1 : 2));
                u32 kbps   = kBitrates[mpeg1 ? 0 : 1][layer - 1][brIdx];
                u32 spf    = layer == 1 ? 384 : ((layer == 3 && !mpeg1) ? 576 : 1152);
                u32 frameLen = layer == 1 ? (12 * kbps * 1000 / rate + pad) * 4 : (spf / 8) * kbps * 1000 / rate + pad;
                bool mono  = (h[3] >> 6) == 3;

                // confirm with the next header so a stray 0xFF in the data doesn't fool us
                if(i + frameLen + 2 <= buf.Size())
                {
                    const u8* n = &buf[i + frameLen];
                    if(n[0] != 0xFF || (n[1] & 0xE0) != 0xE0)
                        continue;
                }

                out.SampleRate = rate;
                out.Channels   = mono ? 1 : 2;
                out.Bitrate    = kbps;
                out.Codec      = layer == 3 ? "mp3" : (layer == 2 ? "mp2" : "mp1");

                u64 frames   = 0;
                u32 xingOff  = 4 + (mpeg1 ? (mono ? 17 : 32) : (mono ? 9 : 17));
                if(i + xingOff + 12 <= buf.Size() && (memcmp(h + xingOff, "Xing", 4) == 0 || memcmp(h + xingOff, "Info", 4) == 0))
                {
                    u32 xflags = BE32(h + xingOff + 4);
                    if(xflags & 1)
                        frames = BE32(h + xingOff + 8);
                }
                else if(i + 36 + 18 <= buf.Size() && memcmp(h + 36, "VBRI", 4) == 0)
                    frames = BE32(h + 36 + 14);

                if(frames > 0)
                {
                    out.Duration = (double)frames * spf / rate;
                    if(out.Duration > 0.0)
                        out.Bitrate = (u32)((file.size - startOffset - i) * 8 / out.Duration / 1000.0);
                }
                else
                {
                    i64 audioBytes = file.size - startOffset - i;
                    u8 tail[3];
                    if(file.size > 128 && file.ReadAt(file.size - 128, tail, 3) && memcmp(tail, "TAG", 3) == 0)
                        audioBytes -= 128;
                    out.Duration = kbps ? (double)audioBytes * 8.0 / (kbps * 1000.0) : 0.0;
                }
                return;
            }
        }

        // ---------------------------------------------------------------- MP4 / M4A

        struct Atom
        {
            u32 Type;
            u64 Offset; // start of payload
            u64 Size;   // payload size
        };

        u32 FourCC(const char* s) { return BE32((const u8*)s); }

        // Iterate child atoms within [begin, end) of a memory buffer.
        template <typename F>
        void ForEachAtom(const u8* base, u64 begin, u64 end, F fn)
        {
            u64 p = begin;
            while(p + 8 <= end)
            {
                u64 size = BE32(base + p);
                u32 type = BE32(base + p + 4);
                u64 hdr  = 8;
                if(size == 1)
                {
                    if(p + 16 > end)
                        break;
                    size = BE64(base + p + 8);
                    hdr  = 16;
                }
                else if(size == 0)
                    size = end - p;
                if(size < hdr || p + size > end)
                    break;
                if(!fn(Atom { type, p + hdr, size - hdr }))
                    break;
                p += size;
            }
        }

        void ParseIlstItem(const u8* base, const Atom& item, AudioTags& out, bool loadArtwork)
        {
            std::string freeform; // "----" items carry their own name
            ForEachAtom(base, item.Offset, item.Offset + item.Size, [&](const Atom& data)
                        {
                if(data.Type == FourCC("name") && data.Size > 4)
                    freeform = StringUtilities::ToLower(std::string((const char*)base + data.Offset + 4, (size_t)data.Size - 4));
                if(data.Type != FourCC("data") || data.Size < 8)
                    return true;
                const u8* p   = base + data.Offset;
                u32 dataType  = BE32(p) & 0xFFFFFF;
                const u8* val = p + 8;
                u64 len       = data.Size - 8;
                std::string text = (dataType == 1) ? Trim(std::string((const char*)val, (size_t)len)) : std::string();
                if(item.Type == FourCC("----"))
                {
                    SetReplayGain(out, freeform, text);
                    return false;
                }

                switch(item.Type)
                {
                case 0xA96E616D: out.Title = text; break;       // ©nam
                case 0xA9415254: out.Artist = text; break;      // ©ART
                case 0xA9616C62: out.Album = text; break;       // ©alb
                case 0x61415254: out.AlbumArtist = text; break; // aART
                case 0xA9777274: out.Composer = text; break;    // ©wrt
                case 0xA967656E: out.Genre = text; break;       // ©gen
                case 0xA9646179: out.Year = ParseYear(text); break; // ©day
                case 0x676E7265:                                // gnre (id3 index + 1)
                    if(len >= 2 && out.Genre.empty())
                    {
                        u32 g = BE16(val);
                        if(g >= 1 && g - 1 < s_GenreCount)
                            out.Genre = s_Genres[g - 1];
                    }
                    break;
                case 0x74726B6E: // trkn
                    if(len >= 6)
                    {
                        out.Track      = BE16(val + 2);
                        out.TrackCount = BE16(val + 4);
                    }
                    break;
                case 0x6469736B: // disk
                    if(len >= 6)
                    {
                        out.Disc      = BE16(val + 2);
                        out.DiscCount = BE16(val + 4);
                    }
                    break;
                case 0xA96C7972: out.Lyrics = text; break;      // ©lyr
                case 0x736F6E6D: out.SortTitle = text; break;   // sonm
                case 0x736F6172: out.SortArtist = text; break;  // soar
                case 0x736F616C: out.SortAlbum = text; break;   // soal
                case 0x736F6161: out.SortAlbumArtist = text; break; // soaa
                case 0x6370696C: // cpil
                    if(len >= 1)
                        out.Compilation = val[0] != 0;
                    break;
                case 0x636F7672: // covr - first image only
                    out.HasArtwork = out.HasArtwork || len > 0;
                    if(loadArtwork && out.Artwork.Empty() && len > 0)
                    {
                        out.Artwork.Resize((size_t)len);
                        memcpy(out.Artwork.Data(), val, (size_t)len);
                        out.ArtworkMime = dataType == 14 ? "image/png" : "image/jpeg";
                    }
                    break;
                }
                return false; });
        }

        // Child atom of `parent` by type (payload offsets into base)
        bool FindChild(const u8* base, const Atom& parent, const char* type, Atom& out)
        {
            bool hit = false;
            ForEachAtom(base, parent.Offset, parent.Offset + parent.Size, [&](const Atom& c)
                        {
                if(c.Type == FourCC(type)) { out = c; hit = true; return false; }
                return true; });
            return hit;
        }

        bool FindPath(const u8* base, Atom cur, std::initializer_list<const char*> path, Atom& out)
        {
            for(const char* name : path)
                if(!FindChild(base, cur, name, cur))
                    return false;
            out = cur;
            return true;
        }

        void SortChapters(TDArray<AudioChapter>& ch)
        {
            for(u32 i = 1; i < ch.Size(); i++)
                for(u32 j = i; j > 0 && ch[j].Start < ch[j - 1].Start; j--)
                {
                    AudioChapter t = ch[j];
                    ch[j]          = ch[j - 1];
                    ch[j - 1]      = t;
                }
        }

        // Nero chapters: moov/udta/chpl (start in 100ns units)
        void ParseChpl(const u8* p, u64 n, AudioTags& out)
        {
            if(n < 5)
                return;
            u64 pos = p[0] == 1 ? 8 : 4; // version 1 has 4 more bytes
            if(pos >= n)
                return;
            u32 count = p[pos++];
            for(u32 i = 0; i < count && pos + 9 <= n; i++)
            {
                u64 start = BE64(p + pos);
                u32 len   = p[pos + 8];
                pos += 9;
                if(pos + len > n)
                    break;
                out.Chapters.PushBack({ std::string((const char*)p + pos, len), (double)start / 1e7 });
                pos += len;
            }
        }

        // QuickTime chapter track: a text track named by another track's tref/chap; each sample is a title
        void ParseChapterTrack(File& file, const u8* base, const TDArray<Atom>& traks, AudioTags& out)
        {
            TDArray<u32> chapIds;
            for(u32 t = 0; t < traks.Size(); t++)
            {
                Atom chap;
                if(FindPath(base, traks[t], { "tref", "chap" }, chap))
                    for(u64 o = 0; o + 4 <= chap.Size; o += 4)
                        chapIds.PushBack(BE32(base + chap.Offset + o));
            }
            if(chapIds.Empty())
                return;
            for(u32 t = 0; t < traks.Size(); t++)
            {
                Atom tkhd;
                if(!FindChild(base, traks[t], "tkhd", tkhd) || tkhd.Size < 24)
                    continue;
                const u8* th = base + tkhd.Offset;
                u32 id       = th[0] == 1 ? BE32(th + 20) : BE32(th + 12);
                bool wanted  = false;
                for(u32 c = 0; c < chapIds.Size(); c++)
                    if(chapIds[c] == id)
                        wanted = true;
                if(!wanted)
                    continue;

                Atom mdhd, stbl, stts, stsz, stsc, stco;
                if(!FindPath(base, traks[t], { "mdia", "mdhd" }, mdhd) || !FindPath(base, traks[t], { "mdia", "minf", "stbl" }, stbl))
                    return;
                const u8* md  = base + mdhd.Offset;
                u32 timescale = md[0] == 1 ? BE32(md + 20) : BE32(md + 12);
                if(!timescale || !FindChild(base, stbl, "stts", stts) || !FindChild(base, stbl, "stsz", stsz) || !FindChild(base, stbl, "stsc", stsc))
                    return;
                bool co64 = false;
                if(!FindChild(base, stbl, "stco", stco))
                {
                    if(!FindChild(base, stbl, "co64", stco))
                        return;
                    co64 = true;
                }

                // sample start times
                TDArray<double> starts;
                const u8* p = base + stts.Offset;
                u32 entries = stts.Size >= 8 ? BE32(p + 4) : 0;
                u64 t0      = 0;
                for(u32 e = 0; e < entries && 8 + (u64)e * 8 + 8 <= stts.Size && starts.Size() < 10000; e++)
                {
                    u32 count = BE32(p + 8 + e * 8), delta = BE32(p + 12 + e * 8);
                    for(u32 k = 0; k < count && starts.Size() < 10000; k++)
                    {
                        starts.PushBack((double)t0 / timescale);
                        t0 += delta;
                    }
                }

                // sample sizes
                const u8* sz  = base + stsz.Offset;
                u32 fixedSize = stsz.Size >= 12 ? BE32(sz + 4) : 0;
                u32 nSamples  = stsz.Size >= 12 ? BE32(sz + 8) : 0;
                auto sampleSize = [&](u32 i) -> u32 { return fixedSize ? fixedSize : (12 + (u64)i * 4 + 4 <= stsz.Size ? BE32(sz + 12 + i * 4) : 0); };

                // sample file offsets: chunk offsets + samples-per-chunk runs
                const u8* sc  = base + stsc.Offset;
                u32 scEntries = stsc.Size >= 8 ? BE32(sc + 4) : 0;
                const u8* co  = base + stco.Offset;
                u32 chunks    = stco.Size >= 8 ? BE32(co + 4) : 0;
                u32 sample    = 0;
                for(u32 c = 0; c < chunks && sample < nSamples; c++)
                {
                    u64 off = co64 ? BE64(co + 8 + (u64)c * 8) : BE32(co + 8 + (u64)c * 4);
                    u32 per = 1;
                    for(u32 e = 0; e < scEntries && 8 + (u64)e * 12 + 12 <= stsc.Size; e++)
                        if(BE32(sc + 8 + e * 12) - 1 <= c)
                            per = BE32(sc + 12 + e * 12);
                    for(u32 k = 0; k < per && sample < nSamples; k++, sample++)
                    {
                        u32 size = sampleSize(sample);
                        u8 buf[514];
                        u32 take = Maths::Min(size, (u32)sizeof(buf));
                        std::string title;
                        if(take >= 2 && file.ReadAt((i64)off, buf, take))
                        {
                            u32 len = Maths::Min((u32)BE16(buf), take - 2);
                            if(len >= 2 && buf[2] == 0xFE && buf[3] == 0xFF)
                                title = DecodeID3Text(2, buf + 4, len - 2);
                            else
                                title.assign((const char*)buf + 2, len);
                        }
                        out.Chapters.PushBack({ title, sample < starts.Size() ? starts[sample] : 0.0 });
                        off += size;
                    }
                }
                return;
            }
        }

        bool ParseMP4(File& file, AudioTags& out, bool loadArtwork)
        {
            u8 hdr[8];
            if(!file.ReadAt(0, hdr, 8) || memcmp(hdr + 4, "ftyp", 4) != 0)
                return false;

            // find moov at the top level without reading mdat
            i64 pos = 0;
            TDArray<u8> moov;
            while(pos + 8 <= file.size)
            {
                u8 h[16];
                if(!file.ReadAt(pos, h, 8))
                    break;
                u64 size = BE32(h);
                u64 hl   = 8;
                if(size == 1)
                {
                    if(!file.ReadAt(pos, h, 16))
                        break;
                    size = BE64(h + 8);
                    hl   = 16;
                }
                else if(size == 0)
                    size = file.size - pos;
                if(size < hl)
                    break;
                if(memcmp(h + 4, "moov", 4) == 0)
                {
                    if(size > 256ull * 1024 * 1024 || !file.ReadAt(pos, moov, (size_t)size))
                        return true;
                    break;
                }
                pos += (i64)size;
            }
            if(moov.Empty())
                return true;

            const u8* base = moov.Data();
            u64 moovHdr    = BE32(base) == 1 ? 16 : 8;
            TDArray<Atom> traks;
            TDArray<AudioChapter> nero;
            ForEachAtom(base, moovHdr, moov.Size(), [&](const Atom& a)
                        {
                if(a.Type == FourCC("trak"))
                    traks.PushBack(a);
                if(a.Type == FourCC("mvhd") && a.Size >= 20)
                {
                    const u8* p = base + a.Offset;
                    u32 timescale;
                    u64 duration;
                    if(p[0] == 1 && a.Size >= 32)
                    {
                        timescale = BE32(p + 20);
                        duration  = BE64(p + 24);
                    }
                    else
                    {
                        timescale = BE32(p + 12);
                        duration  = BE32(p + 16);
                    }
                    if(timescale)
                        out.Duration = (double)duration / timescale;
                }
                else if(a.Type == FourCC("trak") && out.SampleRate == 0)
                {
                    // trak/mdia/minf/stbl/stsd -> first sample entry
                    const char* path[] = { "mdia", "minf", "stbl", "stsd" };
                    Atom cur     = a;
                    bool found   = true;
                    for(const char* name : path)
                    {
                        bool hit = false;
                        ForEachAtom(base, cur.Offset, cur.Offset + cur.Size, [&](const Atom& c)
                                    {
                            if(c.Type == FourCC(name)) { cur = c; hit = true; return false; }
                            return true; });
                        if(!hit)
                        {
                            found = false;
                            break;
                        }
                    }
                    if(found && cur.Size >= 8 + 36)
                    {
                        const u8* e = base + cur.Offset + 8; // skip version/flags + entry count
                        u32 fmt     = BE32(e + 4);
                        if(fmt == FourCC("mp4a") || fmt == FourCC("alac") || fmt == FourCC("ac-3") || fmt == FourCC("Opus") || fmt == FourCC("fLaC"))
                        {
                            out.Channels   = BE16(e + 24);
                            out.SampleRate = BE32(e + 32) >> 16;
                            if(fmt == FourCC("alac") || fmt == FourCC("fLaC"))
                            {
                                out.Codec = fmt == FourCC("alac") ? "alac" : "flac";
                                out.Bits  = BE16(e + 26); // sample size
                            }
                            else
                                out.Codec = fmt == FourCC("mp4a") ? "aac" : (fmt == FourCC("Opus") ? "opus" : "ac3");
                        }
                    }
                }
                if(a.Type == FourCC("udta"))
                {
                    Atom chpl;
                    if(FindChild(base, a, "chpl", chpl))
                    {
                        AudioTags tmp;
                        ParseChpl(base + chpl.Offset, chpl.Size, tmp);
                        nero = std::move(tmp.Chapters);
                    }
                    ForEachAtom(base, a.Offset, a.Offset + a.Size, [&](const Atom& m)
                                {
                        if(m.Type != FourCC("meta"))
                            return true;
                        // meta is usually a full box (4 byte version/flags); QuickTime-style isn't
                        u64 start = m.Offset;
                        if(m.Size >= 12 && BE32(base + m.Offset + 4) != FourCC("hdlr"))
                            start += 4;
                        ForEachAtom(base, start, m.Offset + m.Size, [&](const Atom& il)
                                    {
                            if(il.Type != FourCC("ilst"))
                                return true;
                            ForEachAtom(base, il.Offset, il.Offset + il.Size, [&](const Atom& item)
                                        {
                                ParseIlstItem(base, item, out, loadArtwork);
                                return true; });
                            return false; });
                        return false; });
                }
                return true; });

            // the chapter track wins (what iTunes / Audible-style files use); Nero chapters otherwise
            ParseChapterTrack(file, base, traks, out);
            if(out.Chapters.Empty())
                out.Chapters = std::move(nero);
            SortChapters(out.Chapters);

            if(out.Duration > 0.0)
                out.Bitrate = (u32)(file.size * 8.0 / out.Duration / 1000.0);
            return true;
        }

        // ---------------------------------------------------------------- Vorbis comments (FLAC/Ogg)

        void Base64Decode(const std::string& in, TDArray<u8>& out)
        {
            auto val = [](char c) -> int {
                if(c >= 'A' && c <= 'Z') return c - 'A';
                if(c >= 'a' && c <= 'z') return c - 'a' + 26;
                if(c >= '0' && c <= '9') return c - '0' + 52;
                if(c == '+' || c == '-') return 62;
                if(c == '/' || c == '_') return 63;
                return -1;
            };
            out.Clear();
            u32 acc = 0;
            int bits = 0;
            for(char c : in)
            {
                const int v = val(c);
                if(v < 0)
                    continue;
                acc = (acc << 6) | (u32)v;
                bits += 6;
                if(bits >= 8)
                {
                    bits -= 8;
                    out.PushBack((u8)(acc >> bits));
                }
            }
        }

        // FLAC PICTURE block body (as Ogg keeps it, base64, in METADATA_BLOCK_PICTURE)
        void ParsePictureBlock(const u8* d, size_t n, AudioTags& out)
        {
            auto be = [](const u8* q) { return ((u32)q[0] << 24) | ((u32)q[1] << 16) | ((u32)q[2] << 8) | q[3]; };
            if(n < 32)
                return;
            size_t q          = 4;
            const u32 mimeLen = be(d + q);
            q += 4;
            if(q + mimeLen + 4 > n)
                return;
            std::string mime((const char*)d + q, mimeLen);
            q += mimeLen;
            const u32 descLen = be(d + q);
            q += 4 + descLen + 16;
            if(q + 4 > n)
                return;
            const u32 len = be(d + q);
            q += 4;
            if(q + len > n)
                return;
            out.Artwork.Resize(len);
            memcpy(out.Artwork.Data(), d + q, len);
            out.ArtworkMime = mime;
        }

        void ParseVorbisComments(const u8* p, size_t n, AudioTags& out, bool loadArtwork = false)
        {
            if(n < 8)
                return;
            u32 vendorLen = LE32(p);
            size_t pos    = 4 + (size_t)vendorLen;
            if(pos + 4 > n)
                return;
            u32 count = LE32(p + pos);
            pos += 4;
            for(u32 i = 0; i < count && pos + 4 <= n; i++)
            {
                u32 len = LE32(p + pos);
                pos += 4;
                if(pos + len > n)
                    break;
                std::string kv((const char*)p + pos, len);
                pos += len;
                size_t eq = kv.find('=');
                if(eq == std::string::npos)
                    continue;
                std::string key = StringUtilities::ToLower(kv.substr(0, eq));
                if(key == "metadata_block_picture")
                {
                    out.HasArtwork = true;
                    if(loadArtwork && out.Artwork.Empty())
                    {
                        TDArray<u8> pic;
                        Base64Decode(kv.substr(eq + 1), pic);
                        ParsePictureBlock(pic.Data(), pic.Size(), out);
                    }
                    continue;
                }
                std::string val = Trim(kv.substr(eq + 1));
                if(key == "title")
                    SetIfEmpty(out.Title, val);
                else if(key == "artist")
                    SetIfEmpty(out.Artist, val);
                else if(key == "album")
                    SetIfEmpty(out.Album, val);
                else if(key == "albumartist" || key == "album artist")
                    SetIfEmpty(out.AlbumArtist, val);
                else if(key == "genre")
                    SetIfEmpty(out.Genre, val);
                else if(key == "composer")
                    SetIfEmpty(out.Composer, val);
                else if(key == "tracknumber")
                    ParseNumberPair(val, out.Track, out.TrackCount);
                else if(key == "tracktotal" || key == "totaltracks")
                    out.TrackCount = (u32)atoi(val.c_str());
                else if(key == "discnumber")
                    ParseNumberPair(val, out.Disc, out.DiscCount);
                else if(key == "disctotal" || key == "totaldiscs")
                    out.DiscCount = (u32)atoi(val.c_str());
                else if((key == "date" || key == "year") && !out.Year)
                    out.Year = ParseYear(val);
                else if(key == "compilation")
                    out.Compilation = atoi(val.c_str()) != 0;
                else if(SetReplayGain(out, key, val))
                    continue;
                else if(key == "lyrics" || key == "unsyncedlyrics")
                    SetIfEmpty(out.Lyrics, val);
                else if(key == "titlesort")
                    SetIfEmpty(out.SortTitle, val);
                else if(key == "artistsort")
                    SetIfEmpty(out.SortArtist, val);
                else if(key == "albumsort")
                    SetIfEmpty(out.SortAlbum, val);
                else if(key == "albumartistsort")
                    SetIfEmpty(out.SortAlbumArtist, val);
            }
        }

        bool ParseFLAC(File& file, AudioTags& out, bool loadArtwork)
        {
            u8 magic[4];
            if(!file.ReadAt(0, magic, 4) || memcmp(magic, "fLaC", 4) != 0)
                return false;

            i64 pos = 4;
            bool last = false;
            while(!last && pos + 4 <= file.size)
            {
                u8 h[4];
                if(!file.ReadAt(pos, h, 4))
                    break;
                last     = (h[0] & 0x80) != 0;
                u32 type = h[0] & 0x7F;
                u32 len  = BE24(h + 1);
                pos += 4;

                if(type == 0 && len >= 18) // STREAMINFO
                {
                    u8 si[18];
                    if(file.ReadAt(pos, si, 18))
                    {
                        out.SampleRate = ((u32)si[10] << 12) | ((u32)si[11] << 4) | (si[12] >> 4);
                        out.Channels   = ((si[12] >> 1) & 7) + 1;
                        out.Bits       = (((u32)(si[12] & 1) << 4) | (si[13] >> 4)) + 1;
                        out.Codec      = "flac";
                        u64 samples    = ((u64)(si[13] & 0x0F) << 32) | BE32(si + 14);
                        if(out.SampleRate)
                            out.Duration = (double)samples / out.SampleRate;
                    }
                }
                else if(type == 4) // VORBIS_COMMENT
                {
                    TDArray<u8> buf;
                    if(file.ReadAt(pos, buf, len))
                        ParseVorbisComments(buf.Data(), buf.Size(), out);
                }
                else if(type == 6 && !loadArtwork)
                    out.HasArtwork = true;
                else if(type == 6 && out.Artwork.Empty()) // PICTURE
                {
                    out.HasArtwork = true;
                    TDArray<u8> buf;
                    if(file.ReadAt(pos, buf, len) && len >= 32)
                    {
                        const u8* p = buf.Data();
                        u32 o       = 4;
                        u32 mimeLen = BE32(p + o);
                        o += 4;
                        if(o + mimeLen + 4 <= len)
                        {
                            std::string mime((const char*)p + o, mimeLen);
                            o += mimeLen;
                            u32 descLen = BE32(p + o);
                            o += 4 + descLen + 16;
                            if(o + 4 <= len)
                            {
                                u32 dataLen = BE32(p + o);
                                o += 4;
                                if(o + dataLen <= len)
                                {
                                    out.Artwork.Resize(dataLen);
                                    memcpy(out.Artwork.Data(), p + o, dataLen);
                                    out.ArtworkMime = mime;
                                }
                            }
                        }
                    }
                }
                pos += len;
            }
            if(out.Duration > 0.0)
                out.Bitrate = (u32)(file.size * 8.0 / out.Duration / 1000.0);
            return true;
        }

        bool ParseOgg(File& file, AudioTags& out, bool loadArtwork)
        {
            u8 magic[4];
            if(!file.ReadAt(0, magic, 4) || memcmp(magic, "OggS", 4) != 0)
                return false;

            // Reassemble the first two packets (identification + comment header)
            TDArray<u8> packets[2];
            u32 packetIdx = 0;
            i64 pos       = 0;
            while(packetIdx < 2 && pos + 27 <= file.size)
            {
                u8 ph[27];
                if(!file.ReadAt(pos, ph, 27) || memcmp(ph, "OggS", 4) != 0)
                    break;
                u32 segs = ph[26];
                u8 table[255];
                if(!file.ReadAt(pos + 27, table, segs))
                    break;
                i64 data = pos + 27 + segs;
                for(u32 s = 0; s < segs && packetIdx < 2; s++)
                {
                    u32 lace = table[s];
                    size_t old = packets[packetIdx].Size();
                    if(packets[packetIdx].Size() + lace > 16 * 1024 * 1024)
                        return true;
                    packets[packetIdx].Resize(old + lace);
                    if(lace && !file.ReadAt(data, packets[packetIdx].Data() + old, lace))
                        return true;
                    data += lace;
                    if(lace < 255)
                        packetIdx++;
                }
                pos = data;
            }

            if(packets[0].Size() >= 16 && memcmp(packets[0].Data(), "\x01vorbis", 7) == 0)
            {
                out.Channels   = packets[0][11];
                out.SampleRate = LE32(packets[0].Data() + 12);
                out.Codec      = "vorbis";
            }
            if(packets[1].Size() > 7 && memcmp(packets[1].Data(), "\x03vorbis", 7) == 0)
                ParseVorbisComments(packets[1].Data() + 7, packets[1].Size() - 7, out, loadArtwork);
            u32 preSkip = 0;
            if(packets[0].Size() >= 19 && memcmp(packets[0].Data(), "OpusHead", 8) == 0)
            {
                // Opus always decodes at 48 kHz; granules count 48 kHz samples
                out.Channels   = packets[0][9];
                preSkip        = packets[0][10] | (packets[0][11] << 8);
                out.SampleRate = 48000;
                out.Codec      = "opus";
            }
            if(packets[1].Size() > 8 && memcmp(packets[1].Data(), "OpusTags", 8) == 0)
                ParseVorbisComments(packets[1].Data() + 8, packets[1].Size() - 8, out, loadArtwork);

            // duration = granule position of the last page
            i64 tailSize = Maths::Min<i64>(file.size, 64 * 1024);
            TDArray<u8> tail;
            if(out.SampleRate && file.ReadAt(file.size - tailSize, tail, (size_t)tailSize))
            {
                for(i64 i = (i64)tail.Size() - 14; i >= 0; i--)
                {
                    if(memcmp(&tail[i], "OggS", 4) == 0)
                    {
                        u64 granule = (u64)LE32(&tail[i + 6]) | ((u64)LE32(&tail[i + 10]) << 32);
                        out.Duration = (double)(granule > preSkip ? granule - preSkip : 0) / out.SampleRate;
                        break;
                    }
                }
            }
            if(out.Duration > 0.0)
                out.Bitrate = (u32)(file.size * 8.0 / out.Duration / 1000.0);
            return true;
        }

        // AIFF / AIFC: COMM for the stream info, an "ID3 " chunk for tags
        bool ParseAIFF(File& file, AudioTags& out, bool loadArtwork)
        {
            u8 h[12];
            if(!file.ReadAt(0, h, 12) || memcmp(h, "FORM", 4) || (memcmp(h + 8, "AIFF", 4) && memcmp(h + 8, "AIFC", 4)))
                return false;
            auto be32 = [](const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; };
            out.Codec = "aiff";
            i64 pos   = 12;
            u8 c[8];
            while(pos + 8 <= file.size && file.ReadAt(pos, c, 8))
            {
                const u32 size = be32(c + 4);
                if(!memcmp(c, "COMM", 4) && size >= 18)
                {
                    u8 m[18];
                    if(file.ReadAt(pos + 8, m, 18))
                    {
                        out.Channels     = (u32)((m[0] << 8) | m[1]);
                        const u32 frames = be32(m + 2);
                        out.Bits         = (u32)((m[6] << 8) | m[7]);
                        // 80-bit extended sample rate
                        const int expo = ((m[8] & 0x7F) << 8) | m[9];
                        u64 mant       = 0;
                        for(int i = 0; i < 8; i++)
                            mant = (mant << 8) | m[10 + i];
                        out.SampleRate = (u32)(std::ldexp((double)mant, expo - 16383 - 63) + 0.5);
                        if(out.SampleRate)
                            out.Duration = (double)frames / out.SampleRate;
                        out.Bitrate = out.SampleRate * out.Channels * out.Bits / 1000;
                    }
                }
                else if(!memcmp(c, "ID3 ", 4) || !memcmp(c, "id3 ", 4))
                    ParseID3v2(file, out, loadArtwork, pos + 8);
                pos += 8 + (i64)(size + (size & 1));
            }
            return true;
        }

        // DSD: DSF keeps an ID3v2 tag at the metadata pointer; DFF only gets the stream info
        bool ParseDSD(File& file, AudioTags& out, bool loadArtwork)
        {
            u8 h[92];
            if(file.ReadAt(0, h, 92) && !memcmp(h, "DSD ", 4) && !memcmp(h + 28, "fmt ", 4))
            {
                auto le32 = [](const u8* p) { return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); };
                auto le64 = [&](const u8* p) { return (u64)le32(p) | ((u64)le32(p + 4) << 32); };
                const u64 meta    = le64(h + 20);
                out.Channels      = le32(h + 52);
                out.SampleRate    = le32(h + 56);
                const u64 samples = le64(h + 64);
                if(out.SampleRate)
                    out.Duration = (double)samples / out.SampleRate;
                out.Bitrate = out.SampleRate * out.Channels / 1000;
                out.Codec   = "dsd";
                out.Bits    = 1;
                if(meta > 0)
                    ParseID3v2(file, out, loadArtwork, (i64)meta);
                return true;
            }
            if(file.ReadAt(0, h, 16) && !memcmp(h, "FRM8", 4) && !memcmp(h + 12, "DSD ", 4))
            {
                auto be32 = [](const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; };
                auto be64 = [&](const u8* p) { return ((u64)be32(p) << 32) | be32(p + 4); };
                i64 pos = 16;
                u8 c[12];
                while(file.ReadAt(pos, c, 12))
                {
                    const u64 size = be64(c + 4);
                    if(!memcmp(c, "PROP", 4))
                    {
                        i64 sp = pos + 16, end = pos + 12 + (i64)size;
                        while(sp + 12 <= end && file.ReadAt(sp, c, 12))
                        {
                            const u64 ss = be64(c + 4);
                            u8 v[4]      = {};
                            if(!memcmp(c, "FS  ", 4) && file.ReadAt(sp + 12, v, 4))
                                out.SampleRate = be32(v);
                            else if(!memcmp(c, "CHNL", 4) && file.ReadAt(sp + 12, v, 2))
                                out.Channels = (u32)((v[0] << 8) | v[1]);
                            sp += 12 + (i64)(ss + (ss & 1));
                        }
                    }
                    else if(!memcmp(c, "DSD ", 4) && out.SampleRate && out.Channels)
                    {
                        out.Codec = "dsd";
                        out.Bits  = 1;
                        out.Duration = (double)(size / out.Channels) * 8.0 / out.SampleRate;
                        out.Bitrate  = out.SampleRate * out.Channels / 1000;
                    }
                    pos += 12 + (i64)(size + (size & 1));
                }
                return true;
            }
            return false;
        }

        void ParseWAV(File& file, AudioTags& out, bool loadArtwork)
        {
            u8 h[12];
            if(!file.ReadAt(0, h, 12) || memcmp(h, "RIFF", 4) != 0 || memcmp(h + 8, "WAVE", 4) != 0)
                return;
            i64 pos = 12;
            u32 byteRate = 0;
            while(pos + 8 <= file.size)
            {
                u8 c[8];
                if(!file.ReadAt(pos, c, 8))
                    break;
                u32 size = LE32(c + 4);
                if(memcmp(c, "fmt ", 4) == 0 && size >= 16)
                {
                    u8 f[16];
                    if(file.ReadAt(pos + 8, f, 16))
                    {
                        out.Channels   = f[2] | (f[3] << 8);
                        out.SampleRate = LE32(f + 4);
                        byteRate       = LE32(f + 8);
                        out.Bits       = f[14] | (f[15] << 8);
                        out.Codec      = "wav";
                    }
                }
                else if(memcmp(c, "data", 4) == 0)
                {
                    if(byteRate)
                    {
                        out.Duration = (double)size / byteRate;
                        out.Bitrate  = byteRate * 8 / 1000;
                    }
                    // tags usually come after the audio: keep going
                }
                else if(memcmp(c, "id3 ", 4) == 0 || memcmp(c, "ID3 ", 4) == 0)
                    ParseID3v2(file, out, loadArtwork, pos + 8);
                else if(memcmp(c, "LIST", 4) == 0 && size >= 4)
                {
                    TDArray<u8> list;
                    if(file.ReadAt(pos + 8, list, size) && memcmp(list.Data(), "INFO", 4) == 0)
                    {
                        size_t q = 4;
                        while(q + 8 <= list.Size())
                        {
                            const u32 sl = LE32(list.Data() + q + 4);
                            if(q + 8 + sl > list.Size())
                                break;
                            std::string v((const char*)list.Data() + q + 8, sl);
                            while(!v.empty() && v.back() == 0)
                                v.pop_back();
                            const u8* id = list.Data() + q;
                            if(!memcmp(id, "INAM", 4)) SetIfEmpty(out.Title, v);
                            else if(!memcmp(id, "IART", 4)) SetIfEmpty(out.Artist, v);
                            else if(!memcmp(id, "IPRD", 4)) SetIfEmpty(out.Album, v);
                            else if(!memcmp(id, "IGNR", 4)) SetIfEmpty(out.Genre, v);
                            else if(!memcmp(id, "ICRD", 4) && !out.Year) out.Year = ParseYear(v);
                            else if(!memcmp(id, "ITRK", 4) && !out.Track) out.Track = (u32)atoi(v.c_str());
                            q += 8 + sl + (sl & 1);
                        }
                    }
                }
                pos += 8 + size + (size & 1);
            }
        }

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
    }

    bool ReadAudioTags(const std::string& pathIn, AudioTags& out, bool loadArtwork)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string path = ResolvePath(pathIn);
        File file;
        if(!file.Open(path))
            return false;

        std::string ext = StringUtilities::ToLower(StringUtilities::GetFilePathExtension(path));

        if(!ParseMP4(file, out, loadArtwork) && !ParseFLAC(file, out, loadArtwork) && !ParseOgg(file, out, loadArtwork))
        {
            if(ext == "wav")
                ParseWAV(file, out, loadArtwork);
            else if(ext == "dsf" || ext == "dff")
                ParseDSD(file, out, loadArtwork);
            else if(ext == "aif" || ext == "aiff" || ext == "aifc")
                ParseAIFF(file, out, loadArtwork);
            else
            {
                u32 id3Size = ParseID3v2(file, out, loadArtwork);
                SortChapters(out.Chapters);
                ParseID3v1(file, out);
                if(ext == "mp3" || ext == "mp2" || ext == "mpga" || id3Size > 0)
                    ParseMPEGInfo(file, out, id3Size);
            }
        }

        if(out.Title.empty())
        {
            std::string name = StringUtilities::GetFileName(path);
            size_t dot       = name.find_last_of('.');
            out.Title        = dot == std::string::npos ? name : name.substr(0, dot);
        }
        return true;
    }
}
