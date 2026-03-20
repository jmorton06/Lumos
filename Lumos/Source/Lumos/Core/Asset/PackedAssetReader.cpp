#include "Precompiled.h"
#include "PackedAssetReader.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/Hash.h"

#include <cstdio>

namespace Lumos
{
    PackedAssetReader::PackedAssetReader()
    {
        m_Arena = ArenaAlloc(Megabytes(1));
        HashMapInit(&m_Entries);
    }

    PackedAssetReader::~PackedAssetReader()
    {
        Unmount();
        ArenaRelease(m_Arena);
    }

    bool PackedAssetReader::Mount(const String8& packPath)
    {
        m_PackPath = ToStdString(packPath);

        FILE* file = fopen(m_PackPath.c_str(), "rb");
        if(!file)
        {
            LERROR("PackedAssetReader: Failed to open %.*s", (int)packPath.size, packPath.str);
            return false;
        }

        // Read header
        LPakHeader header;
        if(fread(&header, sizeof(LPakHeader), 1, file) != 1)
        {
            LERROR("PackedAssetReader: Failed to read header %.*s", (int)packPath.size, packPath.str);
            fclose(file);
            return false;
        }

        if(memcmp(header.Magic, LPAK_MAGIC, 4) != 0)
        {
            LERROR("PackedAssetReader: Invalid magic");
            fclose(file);
            return false;
        }

        if(header.Version != LPAK_VERSION)
        {
            LERROR("PackedAssetReader: Version mismatch");
            fclose(file);
            return false;
        }

        // Seek to TOC
        fseek(file, (long)header.TOCOffset, SEEK_SET);

        // Read TOC entries one at a time
        for(u32 i = 0; i < header.EntryCount; i++)
        {
            LPakTOCEntry tocEntry;
            if(fread(&tocEntry, sizeof(LPakTOCEntry), 1, file) != 1)
                break;

            // Read null-terminated path string
            char pathBuf[1024];
            int c, p = 0;
            while((c = fgetc(file)) != EOF && c != '\0' && p < 1023)
                pathBuf[p++] = (char)c;
            pathBuf[p] = '\0';

            PackedEntry entry;
            entry.DataOffset       = tocEntry.DataOffset;
            entry.DataSize         = tocEntry.DataSize;
            entry.UncompressedSize = tocEntry.UncompressedSize;
            entry.AssetType        = tocEntry.AssetType;
            entry.Path             = pathBuf;

            HashMapInsert(&m_Entries, tocEntry.PathHash, entry);
        }

        fclose(file);

        m_Mounted = true;
        LINFO("PackedAssetReader: Mounted %.*s (%u entries)", (int)packPath.size, packPath.str, header.EntryCount);
        return true;
    }

    void PackedAssetReader::Unmount()
    {
        if(m_Mounted)
        {
            HashMapClear(&m_Entries);
            m_Mounted = false;
        }
    }

    bool PackedAssetReader::Contains(const String8& vfsPath) const
    {
        u64 hash = MurmurHash64A((const char*)vfsPath.str, (int)vfsPath.size, 0);
        return HashMapFindPtr(&m_Entries, hash) != nullptr;
    }

    i64 PackedAssetReader::GetAssetSize(const String8& vfsPath) const
    {
        u64 hash = MurmurHash64A((const char*)vfsPath.str, (int)vfsPath.size, 0);
        PackedEntry* entry = (PackedEntry*)HashMapFindPtr(&m_Entries, hash);
        return entry ? (i64)entry->DataSize : -1;
    }

    u8* PackedAssetReader::ReadAsset(Arena* arena, const String8& vfsPath, u64* outSize)
    {
        u64 hash = MurmurHash64A((const char*)vfsPath.str, (int)vfsPath.size, 0);
        PackedEntry* entry = (PackedEntry*)HashMapFindPtr(&m_Entries, hash);
        if(!entry)
            return nullptr;

        FILE* file = fopen(m_PackPath.c_str(), "rb");
        if(!file)
            return nullptr;

        fseek(file, (long)entry->DataOffset, SEEK_SET);

        u8* result = PushArray(arena, u8, entry->DataSize);
        size_t bytesRead = fread(result, 1, entry->DataSize, file);
        fclose(file);

        if(bytesRead != entry->DataSize)
            return nullptr;

        if(outSize)
            *outSize = entry->DataSize;

        return result;
    }
}
