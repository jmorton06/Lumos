#include "Precompiled.h"
#include "AudioTagWriter.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/MathsUtilities.h"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace Lumos
{
    namespace
    {
        using Bytes = TDArray<u8>;

        bool Fail(std::string* error, const char* why)
        {
            if(error)
                *error = why;
            return false;
        }

        void Append(Bytes& b, const void* data, size_t n)
        {
            const size_t at = b.Size();
            b.Resize(at + n);
            if(n)
                memcpy(b.Data() + at, data, n);
        }
        void Append(Bytes& b, const Bytes& other) { Append(b, other.Data(), other.Size()); }
        void Append(Bytes& b, const char* s) { Append(b, s, strlen(s)); }
        void PutU8(Bytes& b, u32 v) { u8 x = (u8)v; Append(b, &x, 1); }
        void PutBE16(Bytes& b, u32 v) { u8 x[2] = { (u8)(v >> 8), (u8)v }; Append(b, x, 2); }
        void PutBE32(Bytes& b, u32 v) { u8 x[4] = { (u8)(v >> 24), (u8)(v >> 16), (u8)(v >> 8), (u8)v }; Append(b, x, 4); }
        void PutLE32(Bytes& b, u32 v) { u8 x[4] = { (u8)v, (u8)(v >> 8), (u8)(v >> 16), (u8)(v >> 24) }; Append(b, x, 4); }
        void PutSyncsafe(Bytes& b, u32 v) { u8 x[4] = { (u8)((v >> 21) & 0x7F), (u8)((v >> 14) & 0x7F), (u8)((v >> 7) & 0x7F), (u8)(v & 0x7F) }; Append(b, x, 4); }
        u32 BE32(const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }
        u64 BE64(const u8* p) { return ((u64)BE32(p) << 32) | BE32(p + 4); }
        u32 LE32(const u8* p) { return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); }
        u32 Syncsafe(const u8* p) { return ((u32)(p[0] & 0x7F) << 21) | ((u32)(p[1] & 0x7F) << 14) | ((u32)(p[2] & 0x7F) << 7) | (p[3] & 0x7F); }
        void SetBE32(u8* p, u32 v) { p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v; }
        void SetBE64(u8* p, u64 v) { SetBE32(p, (u32)(v >> 32)); SetBE32(p + 4, (u32)v); }

        // off_t is 32 bit on Windows
        bool Seek(FILE* f, u64 at)
        {
#if defined(_WIN32)
            return _fseeki64(f, (long long)at, SEEK_SET) == 0;
#else
            return fseeko(f, (off_t)at, SEEK_SET) == 0;
#endif
        }

        bool Truncate(FILE* f, u64 size)
        {
#if defined(_WIN32)
            return _chsize_s(_fileno(f), (long long)size) == 0;
#else
            return ftruncate(fileno(f), (off_t)size) == 0;
#endif
        }

        u64 FileSize(FILE* f)
        {
            fseek(f, 0, SEEK_END);
#if defined(_WIN32)
            long long n = _ftelli64(f);
#else
            long long n = ftello(f);
#endif
            return n < 0 ? 0 : (u64)n;
        }

        bool ReadAt(FILE* f, u64 at, u64 n, Bytes& out)
        {
            out.Resize((size_t)n);
            if(!Seek(f, at))
                return false;
            return n == 0 || fread(out.Data(), 1, (size_t)n, f) == n;
        }

        bool WriteAt(FILE* f, u64 at, const u8* data, size_t n)
        {
            if(!Seek(f, at))
                return false;
            return n == 0 || fwrite(data, 1, n, f) == n;
        }

        bool CopyRange(FILE* src, u64 at, u64 n, FILE* dst)
        {
            if(!Seek(src, at))
                return false;
            static const size_t Chunk = 1 << 20;
            Bytes buf;
            buf.Resize(Chunk);
            while(n > 0)
            {
                size_t want = (size_t)(n < Chunk ? n : Chunk);
                size_t got  = fread(buf.Data(), 1, want, src);
                if(got == 0 || fwrite(buf.Data(), 1, got, dst) != got)
                    return false;
                n -= got;
            }
            return true;
        }

        // head + src[from, end) into a temp file beside the original, which then replaces it
        bool Rewrite(const std::string& path, FILE* src, const Bytes& head, u64 from, std::string* error)
        {
            const u64 size = FileSize(src);
            std::string tmp = path + ".lumostags.tmp";
            FILE* out       = fopen(tmp.c_str(), "wb");
            if(!out)
                return Fail(error, "can't write next to the file");
            bool ok = fwrite(head.Data(), 1, head.Size(), out) == head.Size() && CopyRange(src, from, size > from ? size - from : 0, out);
            ok      = (fflush(out) == 0) && ok;
            fclose(out);
            if(!ok)
            {
                remove(tmp.c_str());
                return Fail(error, "writing failed (disk full?)");
            }
            fclose(src);
            struct stat st;
            if(stat(path.c_str(), &st) == 0)
                chmod(tmp.c_str(), st.st_mode);
            if(rename(tmp.c_str(), path.c_str()) != 0)
            {
                remove(tmp.c_str());
                return Fail(error, "couldn't replace the file");
            }
            return true;
        }

        std::string LowerExt(const std::string& path)
        {
            size_t dot = path.find_last_of('.');
            std::string e = dot == std::string::npos ? "" : path.substr(dot + 1);
            for(char& c : e)
                c = (char)tolower((unsigned char)c);
            return e;
        }

        void Utf8ToUtf16LE(const std::string& s, Bytes& out)
        {
            size_t i = 0;
            while(i < s.size())
            {
                u32 c        = (u8)s[i];
                size_t extra = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
                c            = extra == 3 ? (c & 0x07) : extra == 2 ? (c & 0x0F) : extra == 1 ? (c & 0x1F) : c;
                for(size_t k = 1; k <= extra && i + k < s.size(); k++)
                    c = (c << 6) | ((u8)s[i + k] & 0x3F);
                i += extra + 1;
                if(c >= 0x10000)
                {
                    c -= 0x10000;
                    u32 hi = 0xD800 + (c >> 10), lo = 0xDC00 + (c & 0x3FF);
                    PutU8(out, hi & 0xFF); PutU8(out, hi >> 8);
                    PutU8(out, lo & 0xFF); PutU8(out, lo >> 8);
                }
                else
                {
                    PutU8(out, c & 0xFF); PutU8(out, c >> 8);
                }
            }
        }

        std::string NumPair(u32 n, u32 total)
        {
            if(!n)
                return "";
            char buf[32];
            if(total)
                snprintf(buf, sizeof(buf), "%u/%u", n, total);
            else
                snprintf(buf, sizeof(buf), "%u", n);
            return buf;
        }

        std::string ArtMime(const AudioTagEdit& e)
        {
            if(!e.artworkMime.empty())
                return e.artworkMime;
            const std::string& a = e.artwork;
            if(a.size() > 4 && (u8)a[0] == 0x89 && a[1] == 'P' && a[2] == 'N' && a[3] == 'G')
                return "image/png";
            return "image/jpeg";
        }

        // ------------------------------------------------------------ ID3v2 (mp3)

        void Id3Frame(Bytes& out, const char* id, const Bytes& body, u32 version)
        {
            Append(out, id, 4);
            if(version == 4)
                PutSyncsafe(out, (u32)body.Size());
            else
                PutBE32(out, (u32)body.Size());
            PutBE16(out, 0);
            Append(out, body);
        }

        // text in the frame's encoding: 3 = UTF-8 (v2.4), 1 = UTF-16 with BOM (v2.3)
        void Id3Text(Bytes& body, const std::string& v, u32 version, bool terminate)
        {
            if(version == 4)
            {
                Append(body, v.data(), v.size());
                if(terminate)
                    PutU8(body, 0);
            }
            else
            {
                PutU8(body, 0xFF);
                PutU8(body, 0xFE);
                Utf8ToUtf16LE(v, body);
                if(terminate)
                    PutBE16(body, 0);
            }
        }

        void Id3Lyrics(Bytes& out, const std::string& text, u32 version)
        {
            if(text.empty())
                return;
            Bytes body;
            PutU8(body, version == 4 ? 3 : 1);
            Append(body, "eng", 3);
            Id3Text(body, "", version, true); // content descriptor
            Id3Text(body, text, version, false);
            Id3Frame(out, "USLT", body, version);
        }

        void Id3Picture(Bytes& out, const AudioTagEdit& e, u32 version)
        {
            if(e.artwork.empty())
                return;
            Bytes body;
            PutU8(body, 0); // latin-1 description
            const std::string mime = ArtMime(e);
            Append(body, mime.c_str(), mime.size() + 1);
            PutU8(body, 3); // front cover
            PutU8(body, 0); // empty description
            Append(body, e.artwork.data(), e.artwork.size());
            Id3Frame(out, "APIC", body, version);
        }

        void Id3TextFrame(Bytes& out, const char* id, const std::string& value, u32 version)
        {
            if(value.empty())
                return; // removed
            Bytes body;
            if(version == 4)
            {
                PutU8(body, 3); // UTF-8
                Append(body, value.data(), value.size());
            }
            else
            {
                PutU8(body, 1); // UTF-16 with BOM
                PutU8(body, 0xFF);
                PutU8(body, 0xFE);
                Utf8ToUtf16LE(value, body);
            }
            Append(out, id, 4);
            if(version == 4)
                PutSyncsafe(out, (u32)body.Size());
            else
                PutBE32(out, (u32)body.Size());
            PutBE16(out, 0);
            Append(out, body);
        }

        bool Id3Replaced(const char* id, const AudioTagEdit& e)
        {
            auto is = [&](const char* x) { return memcmp(id, x, 4) == 0; };
            return ((e.set & AudioTagEdit::Title) && is("TIT2")) || ((e.set & AudioTagEdit::Artist) && is("TPE1"))
                || ((e.set & AudioTagEdit::Album) && is("TALB")) || ((e.set & AudioTagEdit::AlbumArtist) && is("TPE2"))
                || ((e.set & AudioTagEdit::Genre) && is("TCON")) || ((e.set & AudioTagEdit::Composer) && is("TCOM"))
                || ((e.set & AudioTagEdit::Year) && (is("TDRC") || is("TYER") || is("TDAT")))
                || ((e.set & AudioTagEdit::TrackNum) && is("TRCK")) || ((e.set & AudioTagEdit::DiscNum) && is("TPOS"))
                || ((e.set & AudioTagEdit::Lyrics) && is("USLT")) || ((e.set & AudioTagEdit::Artwork) && is("APIC"))
                || ((e.set & AudioTagEdit::SortTitle) && is("TSOT")) || ((e.set & AudioTagEdit::SortArtist) && is("TSOP"))
                || ((e.set & AudioTagEdit::SortAlbum) && is("TSOA")) || ((e.set & AudioTagEdit::SortAlbumArtist) && is("TSO2"));
        }

        void Unsync(Bytes& b)
        {
            size_t w = 0;
            for(size_t r = 0; r < b.Size(); r++)
            {
                b[w++] = b[r];
                if(b[r] == 0xFF && r + 1 < b.Size() && b[r + 1] == 0x00)
                    r++;
            }
            b.Resize(w);
        }

        // ID3v2.2 frame ids in their v2.3 names (anything else is dropped when a 2.2 tag is converted)
        const char* Id3v22To23(const char* id)
        {
            static const char* map[][2] = {
                { "TT2", "TIT2" }, { "TP1", "TPE1" }, { "TAL", "TALB" }, { "TP2", "TPE2" }, { "TCO", "TCON" }, { "TCM", "TCOM" },
                { "TRK", "TRCK" }, { "TPA", "TPOS" }, { "TYE", "TYER" }, { "TDA", "TDAT" }, { "TT1", "TIT1" }, { "TT3", "TIT3" },
                { "TP3", "TPE3" }, { "TP4", "TPE4" }, { "TXT", "TEXT" }, { "TPB", "TPUB" }, { "TEN", "TENC" }, { "TCR", "TCOP" },
                { "TBP", "TBPM" }, { "TLA", "TLAN" }, { "TCP", "TCMP" }, { "TST", "TSOT" }, { "TSP", "TSOP" }, { "TSA", "TSOA" },
                { "TS2", "TSO2" }, { "TXX", "TXXX" }, { "COM", "COMM" }, { "ULT", "USLT" }, { "WXX", "WXXX" }, { "UFI", "UFID" },
                { "PCN", "PCNT" }, { "POP", "POPM" }, { "CNT", "PCNT" },
            };
            for(auto& m : map)
                if(memcmp(id, m[0], 3) == 0)
                    return m[1];
            return nullptr;
        }

        // The tag's frames minus the ones being replaced, then the new ones, in the tag's version
        // (a 2.2 tag becomes 2.3). tag = the whole tag from "ID3", or empty for none.
        void BuildID3(const Bytes& tag, const AudioTagEdit& e, Bytes& frames, u32& version)
        {
            version = 4;
            Bytes kept;
            auto keep = [&](const char* id, const u8* data, u32 len, u32 flags) {
                Append(kept, id, 4);
                if(version == 4)
                    PutSyncsafe(kept, len);
                else
                    PutBE32(kept, len);
                PutBE16(kept, flags);
                Append(kept, data, len);
            };
            if(tag.Size() >= 10 && memcmp(tag.Data(), "ID3", 3) == 0)
            {
                const u32 src    = tag[3];
                const u8 flags   = tag[5];
                const u32 size   = Syncsafe(tag.Data() + 6);
                version          = src == 2 ? 3 : src;
                Bytes body;
                Append(body, tag.Data() + 10, Maths::Min<size_t>(size, tag.Size() - 10));
                if((flags & 0x80) && src < 4)
                    Unsync(body); // whole-tag unsynchronisation (2.2 / 2.3)
                size_t p = 0;
                if((flags & 0x40) && src >= 3 && body.Size() >= 4)
                {
                    // extended header: skipped, not written back
                    const u32 ext = src == 4 ? Syncsafe(body.Data()) : BE32(body.Data()) + 4;
                    p             = ext;
                }
                const u32 hdr = src == 2 ? 6 : 10;
                while(p + hdr <= body.Size() && body[p] != 0)
                {
                    const u8* f = body.Data() + p;
                    u32 len     = src == 2 ? (((u32)f[3] << 16) | ((u32)f[4] << 8) | f[5]) : (src == 4 ? Syncsafe(f + 4) : BE32(f + 4));
                    if(p + hdr + len > body.Size())
                        break;
                    const u8* data = f + hdr;
                    if(src == 2)
                    {
                        char id3[4];
                        memcpy(id3, f, 3);
                        const char* id = nullptr;
                        if(memcmp(f, "PIC", 3) == 0)
                        {
                            // PIC: encoding, 3-letter format, type, description, data -> APIC with a mime type
                            if(!(e.set & AudioTagEdit::Artwork) && len > 5)
                            {
                                Bytes apic;
                                PutU8(apic, data[0]);
                                const char* mime = memcmp(data + 1, "PNG", 3) == 0 ? "image/png" : "image/jpeg";
                                Append(apic, mime, strlen(mime) + 1);
                                Append(apic, data + 4, len - 4);
                                keep("APIC", apic.Data(), (u32)apic.Size(), 0);
                            }
                        }
                        else if((id = Id3v22To23((const char*)f)) && !Id3Replaced(id, e))
                            keep(id, data, len, 0);
                        (void)id3;
                    }
                    else
                    {
                        const u32 fflags = ((u32)f[8] << 8) | f[9];
                        if(!Id3Replaced((const char*)f, e))
                        {
                            // 2.4 per-frame unsynchronisation: undo it and clear the flag
                            if(src == 4 && ((fflags & 0x0002) || (flags & 0x80)))
                            {
                                Bytes fd;
                                Append(fd, data, len);
                                Unsync(fd);
                                keep((const char*)f, fd.Data(), (u32)fd.Size(), fflags & ~0x0002u);
                            }
                            else
                                keep((const char*)f, data, len, fflags);
                        }
                    }
                    p += hdr + len;
                }
            }
            frames.Clear();
            Append(frames, kept);
            if(e.set & AudioTagEdit::Title) Id3TextFrame(frames, "TIT2", e.title, version);
            if(e.set & AudioTagEdit::Artist) Id3TextFrame(frames, "TPE1", e.artist, version);
            if(e.set & AudioTagEdit::Album) Id3TextFrame(frames, "TALB", e.album, version);
            if(e.set & AudioTagEdit::AlbumArtist) Id3TextFrame(frames, "TPE2", e.albumArtist, version);
            if(e.set & AudioTagEdit::Genre) Id3TextFrame(frames, "TCON", e.genre, version);
            if(e.set & AudioTagEdit::Composer) Id3TextFrame(frames, "TCOM", e.composer, version);
            if(e.set & AudioTagEdit::Year) Id3TextFrame(frames, version == 4 ? "TDRC" : "TYER", e.year ? std::to_string(e.year) : "", version);
            if(e.set & AudioTagEdit::TrackNum) Id3TextFrame(frames, "TRCK", NumPair(e.track, e.trackCount), version);
            if(e.set & AudioTagEdit::DiscNum) Id3TextFrame(frames, "TPOS", NumPair(e.disc, e.discCount), version);
            if(e.set & AudioTagEdit::SortTitle) Id3TextFrame(frames, "TSOT", e.sortTitle, version);
            if(e.set & AudioTagEdit::SortArtist) Id3TextFrame(frames, "TSOP", e.sortArtist, version);
            if(e.set & AudioTagEdit::SortAlbum) Id3TextFrame(frames, "TSOA", e.sortAlbum, version);
            if(e.set & AudioTagEdit::SortAlbumArtist) Id3TextFrame(frames, "TSO2", e.sortAlbumArtist, version);
            if(e.set & AudioTagEdit::Lyrics) Id3Lyrics(frames, e.lyrics, version);
            if(e.set & AudioTagEdit::Artwork) Id3Picture(frames, e, version);
        }

        void Id3Header(Bytes& tag, u32 version, u32 body)
        {
            Append(tag, "ID3");
            PutU8(tag, version);
            PutU8(tag, 0);
            PutU8(tag, 0); // no unsync / extended header / footer
            PutSyncsafe(tag, body);
        }

        bool WriteID3(const std::string& path, const AudioTagEdit& e, std::string* error)
        {
            FILE* f = fopen(path.c_str(), "r+b");
            if(!f)
                return Fail(error, "can't open the file for writing");
            Bytes hdr, old;
            u32 oldTotal = 0;
            if(ReadAt(f, 0, 10, hdr) && memcmp(hdr.Data(), "ID3", 3) == 0)
            {
                const u32 size = Syncsafe(hdr.Data() + 6);
                oldTotal       = 10 + size + ((hdr[3] == 4 && (hdr[5] & 0x10)) ? 10 : 0);
                ReadAt(f, 0, 10 + size, old);
            }
            Bytes frames;
            u32 version = 4;
            BuildID3(old, e, frames, version);

            // fits in the old tag (its padding): rewrite just the tag
            const bool inPlace = oldTotal >= 10 + frames.Size() && (oldTotal - 10) < (1u << 28);
            const u32 body     = inPlace ? oldTotal - 10 : (u32)frames.Size() + 2048;
            Bytes tag;
            Id3Header(tag, version, body);
            Append(tag, frames);
            tag.Resize(10 + body); // zero padding
            if(inPlace)
            {
                bool ok = WriteAt(f, 0, tag.Data(), tag.Size());
                fclose(f);
                return ok || Fail(error, "writing failed");
            }
            return Rewrite(path, f, tag, oldTotal, error);
        }

        // ------------------------------------------------------------ MP4 (m4a / m4b / alac)

        struct Atom
        {
            u64 off;  // header start
            u64 size; // whole atom
            u32 head; // header bytes (8 or 16)
            char type[5];
        };

        bool ParseAtom(const u8* p, u64 avail, u64 off, Atom& a)
        {
            if(avail < 8)
                return false;
            u64 size = BE32(p);
            a.head   = 8;
            memcpy(a.type, p + 4, 4);
            a.type[4] = 0;
            if(size == 1)
            {
                if(avail < 16)
                    return false;
                size   = BE64(p + 8);
                a.head = 16;
            }
            else if(size == 0)
                size = avail;
            if(size < a.head || size > avail)
                return false;
            a.off  = off;
            a.size = size;
            return true;
        }

        // child of `type` inside buf[from, to)
        bool FindChild(const Bytes& buf, u64 from, u64 to, const char* type, Atom& out)
        {
            u64 p = from;
            while(p + 8 <= to)
            {
                Atom a;
                if(!ParseAtom(buf.Data() + p, to - p, p, a))
                    return false;
                if(memcmp(a.type, type, 4) == 0)
                {
                    out = a;
                    return true;
                }
                p += a.size;
            }
            return false;
        }

        void MakeAtom(Bytes& out, const char* type, const Bytes& body)
        {
            PutBE32(out, (u32)(8 + body.Size()));
            Append(out, type, 4);
            Append(out, body);
        }

        void DataAtom(Bytes& out, const char* type, u32 kind, const Bytes& payload)
        {
            Bytes data;
            PutBE32(data, kind); // 1 = UTF-8 text, 0 = implicit (track / disc numbers)
            PutBE32(data, 0);
            Append(data, payload);
            Bytes item;
            MakeAtom(item, "data", data);
            MakeAtom(out, type, item);
        }

        void TextItem(Bytes& out, const char* type, const std::string& v)
        {
            if(v.empty())
                return;
            Bytes p;
            Append(p, v.data(), v.size());
            DataAtom(out, type, 1, p);
        }

        // ilst items for the edit, plus which item types it replaces
        void BuildItems(const AudioTagEdit& e, Bytes& out, TDArray<std::string>& replaces)
        {
            auto R = [&](const char* t) { replaces.PushBack(std::string(t, 4)); };
            if(e.set & AudioTagEdit::Title) { R("\xA9nam"); TextItem(out, "\xA9nam", e.title); }
            if(e.set & AudioTagEdit::Artist) { R("\xA9" "ART"); TextItem(out, "\xA9" "ART", e.artist); }
            if(e.set & AudioTagEdit::Album) { R("\xA9" "alb"); TextItem(out, "\xA9" "alb", e.album); }
            if(e.set & AudioTagEdit::AlbumArtist) { R("aART"); TextItem(out, "aART", e.albumArtist); }
            if(e.set & AudioTagEdit::Genre) { R("\xA9gen"); R("gnre"); TextItem(out, "\xA9gen", e.genre); }
            if(e.set & AudioTagEdit::Composer) { R("\xA9wrt"); TextItem(out, "\xA9wrt", e.composer); }
            if(e.set & AudioTagEdit::Year) { R("\xA9" "day"); if(e.year) TextItem(out, "\xA9" "day", std::to_string(e.year)); }
            if(e.set & AudioTagEdit::TrackNum)
            {
                R("trkn");
                if(e.track)
                {
                    Bytes p;
                    PutBE16(p, 0); PutBE16(p, e.track); PutBE16(p, e.trackCount); PutBE16(p, 0);
                    DataAtom(out, "trkn", 0, p);
                }
            }
            if(e.set & AudioTagEdit::Lyrics) { R("\xA9lyr"); TextItem(out, "\xA9lyr", e.lyrics); }
            if(e.set & AudioTagEdit::SortTitle) { R("sonm"); TextItem(out, "sonm", e.sortTitle); }
            if(e.set & AudioTagEdit::SortArtist) { R("soar"); TextItem(out, "soar", e.sortArtist); }
            if(e.set & AudioTagEdit::SortAlbum) { R("soal"); TextItem(out, "soal", e.sortAlbum); }
            if(e.set & AudioTagEdit::SortAlbumArtist) { R("soaa"); TextItem(out, "soaa", e.sortAlbumArtist); }
            if(e.set & AudioTagEdit::Artwork)
            {
                R("covr");
                if(!e.artwork.empty())
                {
                    Bytes p;
                    Append(p, e.artwork.data(), e.artwork.size());
                    DataAtom(out, "covr", ArtMime(e) == "image/png" ? 14 : 13, p);
                }
            }
            if(e.set & AudioTagEdit::DiscNum)
            {
                R("disk");
                if(e.disc)
                {
                    Bytes p;
                    PutBE16(p, 0); PutBE16(p, e.disc); PutBE16(p, e.discCount);
                    DataAtom(out, "disk", 0, p);
                }
            }
        }

        void Splice(Bytes& buf, u64 at, u64 removeLen, const Bytes& insert)
        {
            Bytes out;
            Append(out, buf.Data(), (size_t)at);
            Append(out, insert);
            Append(out, buf.Data() + at + removeLen, (size_t)(buf.Size() - at - removeLen));
            buf = out;
        }

        void GrowAtom(Bytes& buf, const Atom& a, i64 delta)
        {
            if(a.head == 16)
                SetBE64(buf.Data() + a.off + 8, (u64)((i64)a.size + delta));
            else
                SetBE32(buf.Data() + a.off, (u32)((i64)a.size + delta));
        }

        // chunk offsets that point past `after` move by delta (the moov in front of mdat grew)
        void ShiftChunkOffsets(Bytes& moov, u64 from, u64 to, u64 after, i64 delta)
        {
            u64 p = from;
            while(p + 8 <= to)
            {
                Atom a;
                if(!ParseAtom(moov.Data() + p, to - p, p, a))
                    return;
                const u64 body = a.off + a.head, end = a.off + a.size;
                if(!memcmp(a.type, "trak", 4) || !memcmp(a.type, "mdia", 4) || !memcmp(a.type, "minf", 4) || !memcmp(a.type, "stbl", 4))
                    ShiftChunkOffsets(moov, body, end, after, delta);
                else if((!memcmp(a.type, "stco", 4) || !memcmp(a.type, "co64", 4)) && end >= body + 8)
                {
                    const bool wide = a.type[2] == '6';
                    u32 count       = BE32(moov.Data() + body + 4);
                    u8* e           = moov.Data() + body + 8;
                    for(u32 i = 0; i < count && e + (wide ? 8 : 4) <= moov.Data() + end; i++, e += wide ? 8 : 4)
                    {
                        u64 v = wide ? BE64(e) : BE32(e);
                        if(v >= after)
                            v = (u64)((i64)v + delta);
                        if(wide)
                            SetBE64(e, v);
                        else
                            SetBE32(e, (u32)v);
                    }
                }
                p = end;
            }
        }

        bool WriteMP4(const std::string& path, const AudioTagEdit& e, std::string* error)
        {
            FILE* f = fopen(path.c_str(), "r+b");
            if(!f)
                return Fail(error, "can't open the file for writing");
            const u64 fileSize = FileSize(f);
            // top level: find moov and what follows it
            Atom moovA = {}, nextA = {};
            bool haveMoov = false, haveNext = false;
            u64 p = 0;
            while(p + 8 <= fileSize)
            {
                Bytes h;
                if(!ReadAt(f, p, 16 <= fileSize - p ? 16 : fileSize - p, h))
                    break;
                Atom a;
                u64 avail = fileSize - p;
                // ParseAtom checks size against what's in the buffer; give it the real remaining size
                u64 size = BE32(h.Data());
                a.head   = 8;
                memcpy(a.type, h.Data() + 4, 4);
                a.type[4] = 0;
                if(size == 1 && h.Size() >= 16)
                {
                    size   = BE64(h.Data() + 8);
                    a.head = 16;
                }
                else if(size == 0)
                    size = avail;
                if(size < a.head || size > avail)
                    break;
                a.off  = p;
                a.size = size;
                if(haveMoov && !haveNext)
                {
                    nextA    = a;
                    haveNext = true;
                }
                if(!memcmp(a.type, "moov", 4))
                {
                    moovA    = a;
                    haveMoov = true;
                }
                p += size;
            }
            if(!haveMoov)
            {
                fclose(f);
                return Fail(error, "no moov atom - not an MP4 file?");
            }
            Bytes moov;
            if(!ReadAt(f, moovA.off, moovA.size, moov))
            {
                fclose(f);
                return Fail(error, "couldn't read the metadata");
            }
            Atom root;
            ParseAtom(moov.Data(), moov.Size(), 0, root);

            Bytes items;
            TDArray<std::string> replaces;
            BuildItems(e, items, replaces);

            Atom udta, meta, ilst;
            if(!FindChild(moov, root.head, root.size, "udta", udta))
            {
                Bytes hdlr, metaBody, udtaBody, ilstA, metaA, udtaA;
                const u8 h[] = { 0, 0, 0, 0, 0, 0, 0, 0, 'm', 'd', 'i', 'r', 'a', 'p', 'p', 'l', 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                Append(hdlr, h, sizeof(h));
                PutBE32(metaBody, 0);
                MakeAtom(metaBody, "hdlr", hdlr);
                MakeAtom(ilstA, "ilst", items);
                Append(metaBody, ilstA);
                MakeAtom(metaA, "meta", metaBody);
                MakeAtom(udtaA, "udta", metaA);
                Splice(moov, root.size, 0, udtaA);
                GrowAtom(moov, root, (i64)udtaA.Size());
            }
            else if(!FindChild(moov, udta.off + udta.head, udta.off + udta.size, "meta", meta))
            {
                Bytes hdlr, metaBody, ilstA, metaA;
                const u8 h[] = { 0, 0, 0, 0, 0, 0, 0, 0, 'm', 'd', 'i', 'r', 'a', 'p', 'p', 'l', 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                Append(hdlr, h, sizeof(h));
                PutBE32(metaBody, 0);
                MakeAtom(metaBody, "hdlr", hdlr);
                MakeAtom(ilstA, "ilst", items);
                Append(metaBody, ilstA);
                MakeAtom(metaA, "meta", metaBody);
                Splice(moov, udta.off + udta.size, 0, metaA);
                GrowAtom(moov, udta, (i64)metaA.Size());
                GrowAtom(moov, root, (i64)metaA.Size());
            }
            else
            {
                const u64 metaKids = meta.off + meta.head + 4; // meta is a full box
                if(!FindChild(moov, metaKids, meta.off + meta.size, "ilst", ilst))
                {
                    Bytes ilstA;
                    MakeAtom(ilstA, "ilst", items);
                    Splice(moov, meta.off + meta.size, 0, ilstA);
                    for(Atom* a : { &meta, &udta, &root })
                        GrowAtom(moov, *a, (i64)ilstA.Size());
                }
                else
                {
                    // keep the items we aren't changing (artwork, sort names, ratings...)
                    Bytes body;
                    u64 q = ilst.off + ilst.head;
                    while(q + 8 <= ilst.off + ilst.size)
                    {
                        Atom it;
                        if(!ParseAtom(moov.Data() + q, ilst.off + ilst.size - q, q, it))
                            break;
                        bool drop = false;
                        for(auto& r : replaces)
                            if(!memcmp(it.type, r.data(), 4))
                                drop = true;
                        if(!drop)
                            Append(body, moov.Data() + q, (size_t)it.size);
                        q += it.size;
                    }
                    Append(body, items);
                    Bytes ilstA;
                    MakeAtom(ilstA, "ilst", body);
                    const i64 delta = (i64)ilstA.Size() - (i64)ilst.size;
                    Splice(moov, ilst.off, ilst.size, ilstA);
                    for(Atom* a : { &meta, &udta, &root })
                        GrowAtom(moov, *a, delta);
                }
            }

            const i64 delta   = (i64)moov.Size() - (i64)moovA.size;
            const bool isLast = moovA.off + moovA.size == fileSize;
            const bool nextFree = haveNext && (!memcmp(nextA.type, "free", 4) || !memcmp(nextA.type, "skip", 4)) && nextA.head == 8;
            if(delta == 0 || (isLast && delta < 0))
            {
                bool ok = WriteAt(f, moovA.off, moov.Data(), moov.Size());
                if(ok && delta < 0)
                    ok = Truncate(f, moovA.off + moov.Size());
                fclose(f);
                return ok || Fail(error, "writing failed");
            }
            if(isLast)
            {
                bool ok = WriteAt(f, moovA.off, moov.Data(), moov.Size());
                fclose(f);
                return ok || Fail(error, "writing failed");
            }
            // a free atom after moov takes up (or gives back) the difference: nothing else moves
            if(nextFree)
            {
                const i64 freeNew = (i64)nextA.size - delta;
                if(freeNew >= 8)
                {
                    Bytes block;
                    Append(block, moov);
                    PutBE32(block, (u32)freeNew);
                    Append(block, "free");
                    block.Resize(block.Size() + (size_t)(freeNew - 8));
                    bool ok = WriteAt(f, moovA.off, block.Data(), block.Size());
                    fclose(f);
                    return ok || Fail(error, "writing failed");
                }
            }
            else if(delta <= -8)
            {
                // shrank: a free atom fills the gap
                Bytes block;
                Append(block, moov);
                PutBE32(block, (u32)(-delta));
                Append(block, "free");
                block.Resize(block.Size() + (size_t)(-delta - 8));
                bool ok = WriteAt(f, moovA.off, block.Data(), block.Size());
                fclose(f);
                return ok || Fail(error, "writing failed");
            }
            // the audio after moov moves: point the chunk offsets at its new place
            Atom newRoot;
            ParseAtom(moov.Data(), moov.Size(), 0, newRoot);
            ShiftChunkOffsets(moov, newRoot.head, newRoot.size, moovA.off + moovA.size, delta);
            Bytes head;
            Bytes before;
            if(!ReadAt(f, 0, moovA.off, before))
            {
                fclose(f);
                return Fail(error, "couldn't read the file");
            }
            Append(head, before);
            Append(head, moov);
            return Rewrite(path, f, head, moovA.off + moovA.size, error);
        }

        // ------------------------------------------------------------ FLAC

        // FLAC PICTURE block body (also what Ogg keeps base64 in METADATA_BLOCK_PICTURE)
        void PictureBlock(const AudioTagEdit& e, Bytes& pic)
        {
            const std::string mime = ArtMime(e);
            PutBE32(pic, 3); // front cover
            PutBE32(pic, (u32)mime.size());
            Append(pic, mime.data(), mime.size());
            PutBE32(pic, 0); // description
            for(int i = 0; i < 4; i++)
                PutBE32(pic, 0); // width, height, depth, colours: unknown
            PutBE32(pic, (u32)e.artwork.size());
            Append(pic, e.artwork.data(), e.artwork.size());
        }

        std::string Base64(const u8* d, size_t n)
        {
            static const char* k = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string out;
            out.reserve((n + 2) / 3 * 4);
            for(size_t i = 0; i < n; i += 3)
            {
                u32 v = (u32)d[i] << 16 | (i + 1 < n ? (u32)d[i + 1] << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
                out += k[(v >> 18) & 63];
                out += k[(v >> 12) & 63];
                out += i + 1 < n ? k[(v >> 6) & 63] : '=';
                out += i + 2 < n ? k[v & 63] : '=';
            }
            return out;
        }

        // Vorbis comments (FLAC block 4, Ogg Vorbis / Opus comment header): the old ones minus the keys
        // being set, then the new values. pictureComment: artwork goes in as METADATA_BLOCK_PICTURE (Ogg).
        void EditVorbisComments(const u8* d, size_t n, const AudioTagEdit& e, bool pictureComment, Bytes& vc)
        {
            std::string vendor = "LumosTunes";
            TDArray<std::string> comments;
            if(d && n >= 8)
            {
                size_t q = 4;
                u32 vl   = LE32(d);
                if(q + vl <= n)
                    vendor.assign((const char*)d + q, vl);
                q += vl;
                u32 count = q + 4 <= n ? LE32(d + q) : 0;
                q += 4;
                for(u32 i = 0; i < count && q + 4 <= n; i++)
                {
                    u32 cl = LE32(d + q);
                    q += 4;
                    if(q + cl > n)
                        break;
                    comments.PushBack(std::string((const char*)d + q, cl));
                    q += cl;
                }
            }
            auto key = [](const std::string& c) {
                std::string k = c.substr(0, c.find('='));
                for(char& ch : k)
                    ch = (char)toupper((unsigned char)ch);
                return k;
            };
            TDArray<std::string> out;
            for(auto& c : comments)
            {
                std::string k = key(c);
                bool drop     = ((e.set & AudioTagEdit::Title) && k == "TITLE") || ((e.set & AudioTagEdit::Artist) && k == "ARTIST")
                            || ((e.set & AudioTagEdit::Album) && k == "ALBUM")
                            || ((e.set & AudioTagEdit::AlbumArtist) && (k == "ALBUMARTIST" || k == "ALBUM ARTIST"))
                            || ((e.set & AudioTagEdit::Genre) && k == "GENRE") || ((e.set & AudioTagEdit::Composer) && k == "COMPOSER")
                            || ((e.set & AudioTagEdit::Year) && (k == "DATE" || k == "YEAR"))
                            || ((e.set & AudioTagEdit::TrackNum) && (k == "TRACKNUMBER" || k == "TRACKTOTAL" || k == "TOTALTRACKS"))
                            || ((e.set & AudioTagEdit::DiscNum) && (k == "DISCNUMBER" || k == "DISCTOTAL" || k == "TOTALDISCS"))
                            || ((e.set & AudioTagEdit::Lyrics) && (k == "LYRICS" || k == "UNSYNCEDLYRICS"))
                            || ((e.set & AudioTagEdit::SortTitle) && k == "TITLESORT") || ((e.set & AudioTagEdit::SortArtist) && k == "ARTISTSORT")
                            || ((e.set & AudioTagEdit::SortAlbum) && k == "ALBUMSORT") || ((e.set & AudioTagEdit::SortAlbumArtist) && k == "ALBUMARTISTSORT")
                            || (pictureComment && (e.set & AudioTagEdit::Artwork) && (k == "METADATA_BLOCK_PICTURE" || k == "COVERART" || k == "COVERARTMIME"));
                if(!drop)
                    out.PushBack(c);
            }
            auto add = [&](const char* k, const std::string& v) { if(!v.empty()) out.PushBack(std::string(k) + "=" + v); };
            if(e.set & AudioTagEdit::Title) add("TITLE", e.title);
            if(e.set & AudioTagEdit::Artist) add("ARTIST", e.artist);
            if(e.set & AudioTagEdit::Album) add("ALBUM", e.album);
            if(e.set & AudioTagEdit::AlbumArtist) add("ALBUMARTIST", e.albumArtist);
            if(e.set & AudioTagEdit::Genre) add("GENRE", e.genre);
            if(e.set & AudioTagEdit::Composer) add("COMPOSER", e.composer);
            if(e.set & AudioTagEdit::Year) add("DATE", e.year ? std::to_string(e.year) : "");
            if(e.set & AudioTagEdit::TrackNum)
            {
                add("TRACKNUMBER", e.track ? std::to_string(e.track) : "");
                add("TRACKTOTAL", e.trackCount ? std::to_string(e.trackCount) : "");
            }
            if(e.set & AudioTagEdit::DiscNum)
            {
                add("DISCNUMBER", e.disc ? std::to_string(e.disc) : "");
                add("DISCTOTAL", e.discCount ? std::to_string(e.discCount) : "");
            }
            if(e.set & AudioTagEdit::Lyrics) add("LYRICS", e.lyrics);
            if(e.set & AudioTagEdit::SortTitle) add("TITLESORT", e.sortTitle);
            if(e.set & AudioTagEdit::SortArtist) add("ARTISTSORT", e.sortArtist);
            if(e.set & AudioTagEdit::SortAlbum) add("ALBUMSORT", e.sortAlbum);
            if(e.set & AudioTagEdit::SortAlbumArtist) add("ALBUMARTISTSORT", e.sortAlbumArtist);
            if(pictureComment && (e.set & AudioTagEdit::Artwork) && !e.artwork.empty())
            {
                Bytes pic;
                PictureBlock(e, pic);
                out.PushBack("METADATA_BLOCK_PICTURE=" + Base64(pic.Data(), pic.Size()));
            }
            PutLE32(vc, (u32)vendor.size());
            Append(vc, vendor.data(), vendor.size());
            PutLE32(vc, (u32)out.Size());
            for(auto& c : out)
            {
                PutLE32(vc, (u32)c.size());
                Append(vc, c.data(), c.size());
            }
        }

        bool WriteFLAC(const std::string& path, const AudioTagEdit& e, std::string* error)
        {
            FILE* f = fopen(path.c_str(), "r+b");
            if(!f)
                return Fail(error, "can't open the file for writing");
            Bytes magic;
            if(!ReadAt(f, 0, 4, magic) || memcmp(magic.Data(), "fLaC", 4) != 0)
            {
                fclose(f);
                return Fail(error, "not a FLAC file");
            }
            struct Block
            {
                u32 type;
                Bytes body;
            };
            TDArray<Block> blocks;
            u64 p = 4;
            bool last = false;
            while(!last)
            {
                Bytes h;
                if(!ReadAt(f, p, 4, h))
                {
                    fclose(f);
                    return Fail(error, "damaged FLAC header");
                }
                last       = (h[0] & 0x80) != 0;
                u32 type   = h[0] & 0x7F;
                u32 len    = ((u32)h[1] << 16) | ((u32)h[2] << 8) | h[3];
                Block b;
                b.type = type;
                if(!ReadAt(f, p + 4, len, b.body))
                {
                    fclose(f);
                    return Fail(error, "damaged FLAC header");
                }
                blocks.PushBack(b);
                p += 4 + len;
            }
            const u64 audioStart = p;

            // the comment block, with the edit applied
            Bytes vc;
            const u8* oldComments = nullptr;
            size_t oldCommentsSize = 0;
            for(auto& b : blocks)
                if(b.type == 4 && b.body.Size() >= 8)
                {
                    oldComments     = b.body.Data();
                    oldCommentsSize = b.body.Size();
                    break;
                }
            EditVorbisComments(oldComments, oldCommentsSize, e, false, vc);

            // new metadata: the old blocks in order with the comments swapped in, padding last
            TDArray<Block> nb;
            bool placed = false;
            const bool newArt = (e.set & AudioTagEdit::Artwork) != 0;
            for(auto& b : blocks)
            {
                if(b.type == 1 || (b.type == 6 && newArt))
                    continue;
                if(b.type == 4)
                {
                    if(!placed)
                        nb.PushBack({ 4, vc });
                    placed = true;
                    continue;
                }
                nb.PushBack(b);
            }
            if(!placed)
                nb.PushBack({ 4, vc });
            if(newArt && !e.artwork.empty())
            {
                Bytes pic;
                PictureBlock(e, pic);
                if(pic.Size() < (1u << 24))
                    nb.PushBack({ 6, pic });
            }
            u64 used = 4;
            for(auto& b : nb)
                used += 4 + b.body.Size();
            // the old padding (or new padding) makes up the difference
            i64 room     = (i64)audioStart - (i64)used;
            bool inPlace = room == 0 || room >= 4;
            u32 padLen   = inPlace ? (room >= 4 ? (u32)(room - 4) : 0) : 4096;
            bool padBlock = inPlace ? room >= 4 : true;
            Bytes meta;
            Append(meta, "fLaC");
            for(u32 i = 0; i < nb.Size(); i++)
            {
                const bool isLast = !padBlock && i + 1 == nb.Size();
                u32 len           = (u32)nb[i].body.Size();
                PutU8(meta, (isLast ? 0x80 : 0) | nb[i].type);
                PutU8(meta, len >> 16); PutU8(meta, len >> 8); PutU8(meta, len);
                Append(meta, nb[i].body);
            }
            if(padBlock)
            {
                PutU8(meta, 0x80 | 1);
                PutU8(meta, padLen >> 16); PutU8(meta, padLen >> 8); PutU8(meta, padLen);
                meta.Resize(meta.Size() + padLen);
            }
            if(inPlace)
            {
                bool ok = WriteAt(f, 0, meta.Data(), meta.Size());
                fclose(f);
                return ok || Fail(error, "writing failed");
            }
            return Rewrite(path, f, meta, audioStart, error);
        }

        // ------------------------------------------------------------ Ogg (Vorbis / Opus)

        u32 OggCrc(const u8* d, size_t n)
        {
            static u32 table[256];
            static bool init = false;
            if(!init)
            {
                for(u32 i = 0; i < 256; i++)
                {
                    u32 r = i << 24;
                    for(int k = 0; k < 8; k++)
                        r = (r & 0x80000000u) ? (r << 1) ^ 0x04C11DB7u : (r << 1);
                    table[i] = r;
                }
                init = true;
            }
            u32 crc = 0;
            for(size_t i = 0; i < n; i++)
                crc = (crc << 8) ^ table[((crc >> 24) ^ d[i]) & 0xFF];
            return crc;
        }

        void OggSetCrc(Bytes& page)
        {
            memset(page.Data() + 22, 0, 4);
            const u32 c = OggCrc(page.Data(), page.Size());
            page[22] = (u8)c;
            page[23] = (u8)(c >> 8);
            page[24] = (u8)(c >> 16);
            page[25] = (u8)(c >> 24);
        }

        // header packets as pages (granule 0); seq numbers from firstSeq
        void OggPaginate(const TDArray<Bytes>& packets, u32 serial, u32 firstSeq, TDArray<Bytes>& pages)
        {
            TDArray<u8> lacing;
            Bytes data;
            bool continued = false;
            u32 seq        = firstSeq;
            auto flush = [&](bool nextContinues) {
                Bytes page;
                Append(page, "OggS");
                PutU8(page, 0);
                PutU8(page, continued ? 1 : 0);
                for(int i = 0; i < 8; i++)
                    PutU8(page, 0); // granule
                PutLE32(page, serial);
                PutLE32(page, seq++);
                PutLE32(page, 0); // crc
                PutU8(page, (u32)lacing.Size());
                Append(page, lacing.Data(), lacing.Size());
                Append(page, data);
                OggSetCrc(page);
                pages.PushBack(page);
                lacing.Clear();
                data.Clear();
                continued = nextContinues;
            };
            for(auto& pkt : packets)
            {
                // 255-byte segments then the remainder (0 when it divides exactly); a full page (255
                // segments) closes and the packet carries on into the next one
                const size_t n    = pkt.Size();
                const size_t full = n / 255;
                size_t off        = 0;
                auto segment = [&](size_t len) {
                    if(lacing.Size() == 255)
                        flush(true);
                    lacing.PushBack((u8)len);
                    Append(data, pkt.Data() + off, len);
                    off += len;
                };
                for(size_t i = 0; i < full; i++)
                    segment(255);
                segment(n % 255);
                flush(false); // every header packet ends its page (the setup header must)
            }
        }

        bool WriteOgg(const std::string& path, const AudioTagEdit& e, std::string* error)
        {
            FILE* f = fopen(path.c_str(), "rb");
            if(!f)
                return Fail(error, "can't open the file");
            const u64 size = FileSize(f);
            // header pages: collect the header packets and where the audio pages begin
            TDArray<Bytes> packets;
            Bytes cur;
            u64 pos       = 0;
            u32 serial    = 0;
            u32 wanted    = 0; // 3 for Vorbis, 2 for Opus
            bool opus     = false;
            Bytes firstPage;
            u32 headerPages = 0;
            while(pos + 27 <= size)
            {
                Bytes h;
                if(!ReadAt(f, pos, 27, h) || memcmp(h.Data(), "OggS", 4) != 0)
                    break;
                const u32 segs = h[26];
                Bytes table;
                ReadAt(f, pos + 27, segs, table);
                u32 body = 0;
                for(u32 i = 0; i < segs; i++)
                    body += table[i];
                Bytes data;
                ReadAt(f, pos + 27 + segs, body, data);
                if(headerPages == 0)
                {
                    serial = LE32(h.Data() + 14);
                    Append(firstPage, h);
                    Append(firstPage, table);
                    Append(firstPage, data);
                }
                size_t off = 0;
                for(u32 i = 0; i < segs; i++)
                {
                    Append(cur, data.Data() + off, table[i]);
                    off += table[i];
                    if(table[i] < 255)
                    {
                        packets.PushBack(cur);
                        cur.Clear();
                        if(packets.Size() == 1)
                        {
                            opus   = packets[0].Size() >= 8 && memcmp(packets[0].Data(), "OpusHead", 8) == 0;
                            wanted = opus ? 2 : 3;
                            if(!opus && !(packets[0].Size() >= 7 && memcmp(packets[0].Data(), "\x01vorbis", 7) == 0))
                            {
                                fclose(f);
                                return Fail(error, "only Ogg Vorbis and Opus can be tagged");
                            }
                        }
                        if(wanted && packets.Size() == wanted && i + 1 != segs)
                        {
                            fclose(f);
                            return Fail(error, "unusual Ogg layout (audio shares the last header page)");
                        }
                    }
                }
                pos += 27 + segs + body;
                headerPages++;
                if(wanted && packets.Size() >= wanted && cur.Size() == 0)
                    break;
            }
            if(!wanted || packets.Size() < wanted)
            {
                fclose(f);
                return Fail(error, "couldn't read the Ogg headers");
            }
            const u64 audioStart = pos;

            // the new comment packet
            const size_t magic = opus ? 8 : 7;
            Bytes vc;
            EditVorbisComments(packets[1].Data() + magic, packets[1].Size() - magic, e, true, vc);
            Bytes comment;
            Append(comment, packets[1].Data(), magic);
            Append(comment, vc);
            if(!opus)
                PutU8(comment, 1); // framing bit
            TDArray<Bytes> hdrPackets;
            hdrPackets.PushBack(comment);
            if(!opus)
                hdrPackets.PushBack(packets[2]);
            TDArray<Bytes> pages;
            OggPaginate(hdrPackets, serial, 1, pages);
            const i64 shift = (i64)(1 + pages.Size()) - (i64)headerPages;

            std::string tmp = path + ".lumostags.tmp";
            FILE* out       = fopen(tmp.c_str(), "wb");
            if(!out)
            {
                fclose(f);
                return Fail(error, "can't write next to the file");
            }
            bool ok = fwrite(firstPage.Data(), 1, firstPage.Size(), out) == firstPage.Size();
            for(auto& pg : pages)
                ok = ok && fwrite(pg.Data(), 1, pg.Size(), out) == pg.Size();
            // audio pages: renumbered (and re-checksummed) when the header page count changed
            pos = audioStart;
            while(ok && pos + 27 <= size)
            {
                Bytes page;
                if(!ReadAt(f, pos, 27, page) || memcmp(page.Data(), "OggS", 4) != 0)
                    break;
                const u32 segs = page[26];
                Bytes rest;
                ReadAt(f, pos + 27, segs, rest);
                u32 body = 0;
                for(u32 i = 0; i < segs; i++)
                    body += rest[i];
                Bytes data;
                ReadAt(f, pos + 27 + segs, body, data);
                Append(page, rest);
                Append(page, data);
                if(shift != 0 && LE32(page.Data() + 14) == serial)
                {
                    const u32 seq = (u32)((i64)LE32(page.Data() + 18) + shift);
                    page[18] = (u8)seq;
                    page[19] = (u8)(seq >> 8);
                    page[20] = (u8)(seq >> 16);
                    page[21] = (u8)(seq >> 24);
                    OggSetCrc(page);
                }
                ok = fwrite(page.Data(), 1, page.Size(), out) == page.Size();
                pos += page.Size();
            }
            ok = (fflush(out) == 0) && ok;
            fclose(out);
            fclose(f);
            if(!ok)
            {
                remove(tmp.c_str());
                return Fail(error, "writing failed (disk full?)");
            }
            struct stat st;
            if(stat(path.c_str(), &st) == 0)
                chmod(tmp.c_str(), st.st_mode);
            if(rename(tmp.c_str(), path.c_str()) != 0)
            {
                remove(tmp.c_str());
                return Fail(error, "couldn't replace the file");
            }
            return true;
        }

        // ------------------------------------------------------------ WAV / AIFF (ID3 chunk)

        // Rebuilds a RIFF (little-endian) or FORM (big-endian) file with a new ID3 chunk at the end;
        // WAV also gets its LIST/INFO names updated (what Windows / older players read)
        bool WriteChunkedID3(const std::string& path, const AudioTagEdit& e, bool riff, std::string* error)
        {
            FILE* f = fopen(path.c_str(), "rb");
            if(!f)
                return Fail(error, "can't open the file");
            const u64 size = FileSize(f);
            Bytes head;
            if(!ReadAt(f, 0, 12, head) || memcmp(head.Data(), riff ? "RIFF" : "FORM", 4) != 0)
            {
                fclose(f);
                return Fail(error, riff ? "not a WAV file" : "not an AIFF file");
            }
            auto rd32 = [&](const u8* p) { return riff ? LE32(p) : BE32(p); };
            struct Chunk
            {
                u64 off;
                u32 size;
                char id[5];
            };
            TDArray<Chunk> chunks;
            Bytes oldTag;
            TDArray<std::string> infoIds, infoVals;
            u64 pos = 12;
            while(pos + 8 <= size)
            {
                Bytes c;
                ReadAt(f, pos, 8, c);
                Chunk ch;
                ch.off  = pos;
                ch.size = rd32(c.Data() + 4);
                memcpy(ch.id, c.Data(), 4);
                ch.id[4] = 0;
                if(pos + 8 + ch.size > size)
                    ch.size = (u32)(size - pos - 8); // truncated file: take what's there
                const bool isId3 = !memcmp(ch.id, "id3 ", 4) || !memcmp(ch.id, "ID3 ", 4);
                bool isInfo      = false;
                if(riff && !memcmp(ch.id, "LIST", 4) && ch.size >= 4)
                {
                    Bytes body;
                    ReadAt(f, pos + 8, ch.size, body);
                    if(!memcmp(body.Data(), "INFO", 4))
                    {
                        isInfo   = true;
                        size_t q = 4;
                        while(q + 8 <= body.Size())
                        {
                            const u32 sl = LE32(body.Data() + q + 4);
                            if(q + 8 + sl > body.Size())
                                break;
                            std::string v((const char*)body.Data() + q + 8, sl);
                            while(!v.empty() && v.back() == 0)
                                v.pop_back();
                            infoIds.PushBack(std::string((const char*)body.Data() + q, 4));
                            infoVals.PushBack(v);
                            q += 8 + sl + (sl & 1);
                        }
                    }
                }
                if(isId3)
                    ReadAt(f, pos + 8, ch.size, oldTag);
                else if(!isInfo)
                    chunks.PushBack(ch);
                pos += 8 + ch.size + (ch.size & 1);
            }

            Bytes frames;
            u32 version = 4;
            BuildID3(oldTag, e, frames, version);
            Bytes tag;
            Id3Header(tag, version, (u32)frames.Size());
            Append(tag, frames);

            auto put32 = [&](Bytes& b, u32 v) { if(riff) PutLE32(b, v); else PutBE32(b, v); };
            Bytes tail;
            if(riff)
            {
                auto setInfo = [&](const char* id, const std::string& v) {
                    for(u32 i = 0; i < infoIds.Size(); i++)
                        if(infoIds[i] == id)
                        {
                            infoVals[i] = v;
                            return;
                        }
                    infoIds.PushBack(id);
                    infoVals.PushBack(v);
                };
                if(e.set & AudioTagEdit::Title) setInfo("INAM", e.title);
                if(e.set & AudioTagEdit::Artist) setInfo("IART", e.artist);
                if(e.set & AudioTagEdit::Album) setInfo("IPRD", e.album);
                if(e.set & AudioTagEdit::Genre) setInfo("IGNR", e.genre);
                if(e.set & AudioTagEdit::Year) setInfo("ICRD", e.year ? std::to_string(e.year) : "");
                if(e.set & AudioTagEdit::TrackNum) setInfo("ITRK", e.track ? std::to_string(e.track) : "");
                Bytes info;
                Append(info, "INFO");
                for(u32 i = 0; i < infoIds.Size(); i++)
                {
                    if(infoVals[i].empty())
                        continue;
                    Append(info, infoIds[i].data(), 4);
                    PutLE32(info, (u32)infoVals[i].size() + 1);
                    Append(info, infoVals[i].c_str(), infoVals[i].size() + 1);
                    if((infoVals[i].size() + 1) & 1)
                        PutU8(info, 0);
                }
                if(info.Size() > 4)
                {
                    Append(tail, "LIST");
                    put32(tail, (u32)info.Size());
                    Append(tail, info);
                }
            }
            Append(tail, riff ? "id3 " : "ID3 ");
            put32(tail, (u32)tag.Size());
            Append(tail, tag);
            if(tag.Size() & 1)
                PutU8(tail, 0);

            u64 total = 4; // form type
            for(auto& c : chunks)
                total += 8 + c.size + (c.size & 1);
            total += tail.Size();
            if(total > 0xFFFFFFFFull)
            {
                fclose(f);
                return Fail(error, "file too large to retag");
            }
            std::string tmp = path + ".lumostags.tmp";
            FILE* out       = fopen(tmp.c_str(), "wb");
            if(!out)
            {
                fclose(f);
                return Fail(error, "can't write next to the file");
            }
            Bytes hdr;
            Append(hdr, riff ? "RIFF" : "FORM");
            put32(hdr, (u32)total);
            Append(hdr, head.Data() + 8, 4);
            bool ok = fwrite(hdr.Data(), 1, hdr.Size(), out) == hdr.Size();
            for(auto& c : chunks)
            {
                if(!ok)
                    break;
                ok = CopyRange(f, c.off, 8 + (u64)c.size, out);
                if(ok && (c.size & 1))
                    ok = fputc(0, out) != EOF;
            }
            ok = ok && fwrite(tail.Data(), 1, tail.Size(), out) == tail.Size();
            ok = (fflush(out) == 0) && ok;
            fclose(out);
            fclose(f);
            if(!ok)
            {
                remove(tmp.c_str());
                return Fail(error, "writing failed (disk full?)");
            }
            struct stat st;
            if(stat(path.c_str(), &st) == 0)
                chmod(tmp.c_str(), st.st_mode);
            if(rename(tmp.c_str(), path.c_str()) != 0)
            {
                remove(tmp.c_str());
                return Fail(error, "couldn't replace the file");
            }
            return true;
        }
    }

    bool WriteAudioTags(const std::string& path, const AudioTagEdit& edit, std::string* error)
    {
        if(edit.set == 0)
            return true;
        const std::string e = LowerExt(path);
        if(e == "mp3")
            return WriteID3(path, edit, error);
        if(e == "m4a" || e == "m4b" || e == "mp4" || e == "aac" || e == "alac" || e == "m4p")
            return WriteMP4(path, edit, error);
        if(e == "flac")
            return WriteFLAC(path, edit, error);
        if(e == "ogg" || e == "oga" || e == "opus")
            return WriteOgg(path, edit, error);
        if(e == "wav")
            return WriteChunkedID3(path, edit, true, error);
        if(e == "aif" || e == "aiff" || e == "aifc")
            return WriteChunkedID3(path, edit, false, error);
        return Fail(error, "writing tags isn't supported for this format yet");
    }
}
