#include "Precompiled.h"
#include "CSV.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/Timer.h"
#include <cstdlib>
#include <cstring>

namespace Lumos
{
    static const uint8_t* ParseRecord(const uint8_t* p, const uint8_t* end, String8* out, uint32_t maxCols, uint32_t* outCount)
    {
        uint32_t col          = 0;
        bool inQuotes         = false;
        bool quoted           = false;
        const uint8_t* fStart = p;

        auto commit = [&](const uint8_t* fEnd)
        {
            if(col < maxCols)
            {
                const uint8_t* s = fStart;
                const uint8_t* e = fEnd;
                if(quoted)
                {
                    if(s < e && *s == '"')
                        s++;
                    if(e > s && *(e - 1) == '"')
                        e--;
                }
                if(e > s && *(e - 1) == '\r')
                    e--;
                out[col].str  = (uint8_t*)s;
                out[col].size = (uint64_t)(e - s);
            }
            col++;
        };

        while(p < end)
        {
            uint8_t c = *p;
            if(inQuotes)
            {
                if(c == '"')
                {
                    if(p + 1 < end && *(p + 1) == '"') // escaped quote, skip both
                        p++;
                    else
                        inQuotes = false;
                }
            }
            else
            {
                if(c == '"' && p == fStart)
                {
                    quoted   = true;
                    inQuotes = true;
                }
                else if(c == ',')
                {
                    commit(p);
                    fStart = p + 1;
                    quoted = false;
                }
                else if(c == '\n')
                {
                    commit(p);
                    p++;
                    *outCount = col;
                    return p;
                }
            }
            p++;
        }

        // EOF without trailing newline
        if(p > fStart || col > 0)
            commit(p);
        *outCount = col;
        return p;
    }

    CSV CSVParse(Arena* arena, String8 text, bool hasHeader)
    {
        CSV csv      = {};
        csv.source   = text;
        if(text.size == 0)
            return csv;

        const uint8_t* begin = text.str;
        const uint8_t* end   = text.str + text.size;

        // Pass 1: count records, derive column count from first record.
        uint32_t recordCount = 0;
        uint32_t columnCount = 0;
        {
            const uint8_t* p = begin;
            bool inQuotes    = false;
            uint32_t cols    = 1;
            bool counting    = true; // counting columns of first record
            bool recordHasContent = false;
            while(p < end)
            {
                uint8_t c = *p;
                if(c == '"')
                {
                    if(inQuotes && p + 1 < end && *(p + 1) == '"')
                        p++;
                    else
                        inQuotes = !inQuotes;
                    recordHasContent = true;
                }
                else if(!inQuotes && c == ',')
                {
                    if(counting)
                        cols++;
                    recordHasContent = true;
                }
                else if(!inQuotes && c == '\n')
                {
                    recordCount++;
                    counting         = false;
                    recordHasContent = false;
                }
                else if(c != '\r')
                {
                    recordHasContent = true;
                }
                p++;
            }
            if(recordHasContent) // last line without newline
                recordCount++;
            columnCount = cols;
        }

        if(recordCount == 0 || columnCount == 0)
            return csv;

        csv.columnCount = columnCount;

        // Pass 2: fill slices.
        const uint8_t* p = begin;
        if(hasHeader)
        {
            csv.headers = PushArray(arena, String8, columnCount);
            uint32_t n  = 0;
            p           = ParseRecord(p, end, csv.headers, columnCount, &n);
            recordCount = recordCount > 0 ? recordCount - 1 : 0;
        }

        csv.rowCount = recordCount;
        csv.fields   = recordCount ? PushArray(arena, String8, (uint64_t)recordCount * columnCount) : nullptr;

        for(uint32_t r = 0; r < recordCount && p < end; r++)
        {
            uint32_t n = 0;
            p          = ParseRecord(p, end, csv.fields + (uint64_t)r * columnCount, columnCount, &n);
        }

        return csv;
    }

    CSV CSVParseFile(Arena* arena, const String8& vfsPath, bool hasHeader)
    {
        String8 text = FileSystem::Get().ReadTextFileVFS(arena, vfsPath);
        return CSVParse(arena, text, hasHeader);
    }

    static const uint32_t LCSV_MAGIC   = 0x4C435356; // 'LCSV'
    static const uint32_t LCSV_VERSION = 1;

    struct LCSVEntry
    {
        uint32_t off;
        uint32_t size;
    };

    bool CSVWriteBaked(const CSV& csv, const String8& bakedVfsPath, uint64_t sourceSize)
    {
        if(csv.rowCount == 0 || csv.columnCount == 0)
            return false;

        uint64_t headerEntries = csv.headers ? csv.columnCount : 0;
        uint64_t fieldEntries  = (uint64_t)csv.rowCount * csv.columnCount;

        uint64_t blobSize = 0;
        for(uint64_t i = 0; i < headerEntries; i++)
            blobSize += csv.headers[i].size;
        for(uint64_t i = 0; i < fieldEntries; i++)
            blobSize += csv.fields[i].size;
        if(blobSize > 0xFFFFFFFFull)
            return false; // u32 offsets - a >4GB blob has bigger problems

        uint64_t headBytes = 4 + 4 + 8 + 4 + 4 + 1 + 8;
        uint64_t total     = headBytes + (headerEntries + fieldEntries) * sizeof(LCSVEntry) + blobSize;
        if(total > 0xFFFFFFFFull)
            return false; // WriteFileVFS takes u32

        Arena* arena = ArenaAlloc(total + 4096);
        uint8_t* buf = PushArrayNoZero(arena, uint8_t, total);
        uint8_t* p   = buf;

        auto putU32 = [&p](uint32_t v)
        { memcpy(p, &v, 4); p += 4; };
        auto putU64 = [&p](uint64_t v)
        { memcpy(p, &v, 8); p += 8; };

        putU32(LCSV_MAGIC);
        putU32(LCSV_VERSION);
        putU64(sourceSize);
        putU32(csv.rowCount);
        putU32(csv.columnCount);
        *p++ = csv.headers ? 1 : 0;
        putU64(blobSize);

        LCSVEntry* entries = (LCSVEntry*)p;
        p += (headerEntries + fieldEntries) * sizeof(LCSVEntry);
        uint8_t* blob   = p;
        uint32_t cursor = 0;

        auto put = [&](const String8& s, uint64_t entryIdx)
        {
            LCSVEntry e = { cursor, (uint32_t)s.size };
            memcpy(&entries[entryIdx], &e, sizeof(LCSVEntry));
            if(s.size)
            {
                memcpy(blob + cursor, s.str, s.size);
                cursor += (uint32_t)s.size;
            }
        };
        for(uint64_t i = 0; i < headerEntries; i++)
            put(csv.headers[i], i);
        for(uint64_t i = 0; i < fieldEntries; i++)
            put(csv.fields[i], headerEntries + i);

        ArenaTemp temp = ScratchBegin(nullptr, 0);
        String8 phys   = {};
        FileSystem::Get().ResolvePhysicalPath(temp.arena, bakedVfsPath, &phys);
        bool ok = phys.size > 0 && FileSystem::WriteFile(phys, buf, (uint32_t)total);
        ScratchEnd(temp);
        ArenaRelease(arena);
        return ok;
    }

    CSV CSVLoadBaked(Arena* arena, const String8& bakedVfsPath, int64_t expectSourceSize)
    {
        CSV csv = {};
        FileSystem& fs = FileSystem::Get();

        int64_t fileSize = fs.GetFileSizeVFS(bakedVfsPath);
        const int64_t minHead = 4 + 4 + 8 + 4 + 4 + 1 + 8;
        if(fileSize < minHead)
            return csv;

        uint8_t* buf = fs.ReadFileVFS(arena, bakedVfsPath);
        if(!buf)
            return csv;

        const uint8_t* p = buf;
        auto getU32 = [&p]()
        { uint32_t v; memcpy(&v, p, 4); p += 4; return v; };
        auto getU64 = [&p]()
        { uint64_t v; memcpy(&v, p, 8); p += 8; return v; };

        if(getU32() != LCSV_MAGIC || getU32() != LCSV_VERSION)
            return csv;
        uint64_t sourceSize = getU64();
        uint32_t rows       = getU32();
        uint32_t cols       = getU32();
        bool hasHeaders     = *p++ != 0;
        uint64_t blobSize   = getU64();

        if(expectSourceSize >= 0 && sourceSize != (uint64_t)expectSourceSize)
            return csv;
        if(rows == 0 || cols == 0)
            return csv;

        uint64_t headerEntries = hasHeaders ? cols : 0;
        uint64_t fieldEntries  = (uint64_t)rows * cols;
        uint64_t expectTotal   = (uint64_t)minHead + (headerEntries + fieldEntries) * sizeof(LCSVEntry) + blobSize;
        if((uint64_t)fileSize != expectTotal)
            return csv;

        const LCSVEntry* entries = (const LCSVEntry*)p;
        uint8_t* blob            = (uint8_t*)(p + (headerEntries + fieldEntries) * sizeof(LCSVEntry));

        String8* headers = hasHeaders ? PushArray(arena, String8, cols) : nullptr;
        String8* fields  = PushArray(arena, String8, fieldEntries);
        for(uint64_t i = 0; i < headerEntries + fieldEntries; i++)
        {
            LCSVEntry e;
            memcpy(&e, &entries[i], sizeof(LCSVEntry));
            if((uint64_t)e.off + e.size > blobSize)
                return CSV {};
            String8 s = { blob + e.off, e.size };
            if(i < headerEntries)
                headers[i] = s;
            else
                fields[i - headerEntries] = s;
        }

        csv.source      = { blob, blobSize };
        csv.headers     = headers;
        csv.fields      = fields;
        csv.columnCount = cols;
        csv.rowCount    = rows;
        return csv;
    }

    CSV CSVParseFileCached(Arena* arena, const String8& vfsPath, bool hasHeader)
    {
        ArenaTemp scratch = ScratchBegin(&arena, 1);
        String8 bakedPath = PushStr8F(scratch.arena, "%.*s.lcsv", (int)vfsPath.size, (const char*)vfsPath.str);

        int64_t srcSize = FileSystem::Get().GetFileSizeVFS(vfsPath);

        Timer timer;
        CSV baked = CSVLoadBaked(arena, bakedPath, srcSize > 0 ? srcSize : -1);
        if(baked.rowCount)
        {
            LINFO("[CSV] baked cache hit %.*s (%u rows, %.1f ms)", (int)bakedPath.size, (const char*)bakedPath.str, baked.rowCount, timer.GetElapsedMSD());
            ScratchEnd(scratch);
            return baked;
        }

        CSV csv = CSVParseFile(arena, vfsPath, hasHeader);
        LINFO("[CSV] parsed %.*s (%u rows, %.1f ms)", (int)vfsPath.size, (const char*)vfsPath.str, csv.rowCount, timer.GetElapsedMSD());
        if(csv.rowCount)
        {
            Timer bakeTimer;
            uint64_t key = srcSize > 0 ? (uint64_t)srcSize : csv.source.size;
            if(CSVWriteBaked(csv, bakedPath, key))
                LINFO("[CSV] baked %.*s (%.1f ms) - next boot skips the parse", (int)bakedPath.size, (const char*)bakedPath.str, bakeTimer.GetElapsedMSD());
        }
        ScratchEnd(scratch);
        return csv;
    }

    int32_t CSVColumn(const CSV& csv, String8 name)
    {
        for(uint32_t i = 0; i < csv.columnCount; i++)
        {
            if(csv.headers[i] == name)
                return (int32_t)i;
        }
        return -1;
    }

    String8 CSVGet(const CSV& csv, uint32_t row, uint32_t col)
    {
        if(row >= csv.rowCount || col >= csv.columnCount)
            return Str8Lit("");
        return csv.fields[(uint64_t)row * csv.columnCount + col];
    }

    double CSVF64(const CSV& csv, uint32_t row, uint32_t col)
    {
        String8 f = CSVGet(csv, row, col);
        if(f.size == 0)
            return 0.0;
        char buf[64];
        uint64_t n = f.size < 63 ? f.size : 63;
        memcpy(buf, f.str, n);
        buf[n] = 0;
        return strtod(buf, nullptr);
    }

    int64_t CSVI64(const CSV& csv, uint32_t row, uint32_t col)
    {
        String8 f = CSVGet(csv, row, col);
        if(f.size == 0)
            return 0;
        char buf[64];
        uint64_t n = f.size < 63 ? f.size : 63;
        memcpy(buf, f.str, n);
        buf[n] = 0;
        return (int64_t)strtoll(buf, nullptr, 10);
    }
}
