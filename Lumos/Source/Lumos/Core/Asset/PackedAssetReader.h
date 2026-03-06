#pragma once
#include "AssetPacker.h"
#include "Core/DataStructures/Map.h"

namespace Lumos
{
    struct PackedEntry
    {
        u64 DataOffset;
        u64 DataSize;
        u64 UncompressedSize;
        u32 AssetType;
        std::string Path;
    };

    class PackedAssetReader
    {
    public:
        PackedAssetReader();
        ~PackedAssetReader();

        bool Mount(const String8& packPath);
        void Unmount();

        bool Contains(const String8& vfsPath) const;
        u8* ReadAsset(Arena* arena, const String8& vfsPath, u64* outSize);
        i64 GetAssetSize(const String8& vfsPath) const;

        bool IsMounted() const { return m_Mounted; }

    private:
        std::string m_PackPath;
        HashMap(u64, PackedEntry) m_Entries;
        Arena* m_Arena = nullptr;
        bool m_Mounted = false;
    };
}
