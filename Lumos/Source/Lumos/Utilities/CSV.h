#pragma once
#include "Core/String.h"
#include "Core/OS/Memory.h"

namespace Lumos
{
    struct CSV
    {
        String8  source;      // owning text buffer
        String8* headers;     // [columnCount], empty if parsed without header
        String8* fields;      // [rowCount * columnCount], row-major
        uint32_t columnCount;
        uint32_t rowCount;
    };

    CSV     CSVParse(Arena* arena, String8 text, bool hasHeader = true);
    CSV     CSVParseFile(Arena* arena, const String8& vfsPath, bool hasHeader = true);

    CSV     CSVParseFileCached(Arena* arena, const String8& vfsPath, bool hasHeader = true);
    bool    CSVWriteBaked(const CSV& csv, const String8& bakedVfsPath, uint64_t sourceSize);
    CSV     CSVLoadBaked(Arena* arena, const String8& bakedVfsPath, int64_t expectSourceSize); // rowCount 0 = miss

    int32_t CSVColumn(const CSV& csv, String8 name); // header index, -1 if missing
    String8 CSVGet(const CSV& csv, uint32_t row, uint32_t col);
    double  CSVF64(const CSV& csv, uint32_t row, uint32_t col); // 0 on empty/non-numeric
    int64_t CSVI64(const CSV& csv, uint32_t row, uint32_t col);
}
