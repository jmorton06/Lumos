#include "Precompiled.h"
#include "AssetPacker.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/Hash.h"

#include <filesystem>
#include <cstdio>

namespace Lumos
{
    struct PackEntry
    {
        std::string Path;
        u64 PathHash;
        u64 UUID;
        u32 AssetType;
        std::vector<u8> Data;
    };

    struct GatheredFile
    {
        std::string PhysicalPath;
        std::string VFSPath; // //Assets/...
    };

    static void GatherFiles(const std::string& dir, const PackSettings& settings, std::vector<GatheredFile>& outFiles)
    {
        std::string assetsDirNorm = dir;
        // Ensure trailing slash
        if(!assetsDirNorm.empty() && assetsDirNorm.back() != '/' && assetsDirNorm.back() != '\\')
            assetsDirNorm += '/';

        for(auto& entry : std::filesystem::recursive_directory_iterator(assetsDirNorm))
        {
            if(!entry.is_regular_file())
                continue;

            std::string path = entry.path().string();

            // Skip .meta files
            if(StringUtilities::GetFilePathExtension(path) == "meta")
                continue;

            // Compute VFS path
            std::string relative = path.substr(assetsDirNorm.size());
            std::replace(relative.begin(), relative.end(), '\\', '/');
            std::string vfsPath = "//Assets/" + relative;

            // Apply ExcludePaths
            bool excluded = false;
            for(size_t i = 0; i < settings.ExcludePaths.Size(); i++)
            {
                std::string ex((const char*)settings.ExcludePaths[i].str, settings.ExcludePaths[i].size);
                if(vfsPath.find(ex) != std::string::npos)
                {
                    excluded = true;
                    break;
                }
            }
            if(excluded)
                continue;

            outFiles.push_back({ path, vfsPath });
        }
    }

    bool AssetPacker::Pack(const String8& outputPath, const String8& assetsDir, const PackSettings& settings)
    {
        std::string assetsDirStr = ToStdString(assetsDir);
        std::vector<GatheredFile> files;
        GatherFiles(assetsDirStr, settings, files);

        if(files.empty())
        {
            LERROR("AssetPacker: No files found in %s", assetsDirStr.c_str());
            return false;
        }

        // Read all files
        std::vector<PackEntry> entries;
        entries.reserve(files.size());

        for(auto& gf : files)
        {
            FILE* f = fopen(gf.PhysicalPath.c_str(), "rb");
            if(!f)
                continue;

            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);

            if(size <= 0)
            {
                fclose(f);
                continue;
            }

            PackEntry entry;
            entry.Path      = gf.VFSPath;
            entry.PathHash  = MurmurHash64A(gf.VFSPath.c_str(), (int)gf.VFSPath.size(), 0);
            entry.UUID      = entry.PathHash;
            entry.AssetType = 0;
            entry.Data.resize(size);

            size_t bytesRead = fread(entry.Data.data(), 1, size, f);
            fclose(f);

            if((long)bytesRead != size)
                continue;

            entries.push_back(std::move(entry));
        }

        // Calculate layout
        u64 currentDataOffset = sizeof(LPakHeader);
        currentDataOffset = (currentDataOffset + 15) & ~15ULL;

        struct EntryLayout
        {
            u64 offset;
            u64 size;
        };
        std::vector<EntryLayout> layouts(entries.size());

        for(size_t i = 0; i < entries.size(); i++)
        {
            layouts[i].offset = currentDataOffset;
            layouts[i].size   = entries[i].Data.size();
            currentDataOffset += layouts[i].size;
            currentDataOffset = (currentDataOffset + 15) & ~15ULL;
        }

        u64 tocOffset = currentDataOffset;

        // Calculate TOC size
        u64 tocSize = 0;
        for(auto& entry : entries)
        {
            tocSize += sizeof(LPakTOCEntry);
            tocSize += entry.Path.size() + 1; // null terminated path
        }

        u64 totalSize = tocOffset + tocSize;

        // Build output buffer
        std::vector<u8> buffer(totalSize, 0);

        // Header
        LPakHeader header = {};
        memcpy(header.Magic, LPAK_MAGIC, 4);
        header.Version    = LPAK_VERSION;
        header.EntryCount = (u32)entries.size();
        header.TOCOffset  = (u32)tocOffset;
        header.TotalSize  = totalSize;
        header.Flags      = 0;
        header.Reserved   = 0;

        memcpy(buffer.data(), &header, sizeof(LPakHeader));

        // Data blocks
        for(size_t i = 0; i < entries.size(); i++)
        {
            memcpy(buffer.data() + layouts[i].offset, entries[i].Data.data(), entries[i].Data.size());
        }

        // TOC entries
        u8* tocPtr = buffer.data() + tocOffset;
        for(size_t i = 0; i < entries.size(); i++)
        {
            LPakTOCEntry tocEntry = {};
            tocEntry.UUID              = entries[i].PathHash;
            tocEntry.AssetType         = entries[i].AssetType;
            tocEntry.DataOffset        = layouts[i].offset;
            tocEntry.DataSize          = layouts[i].size;
            tocEntry.UncompressedSize  = layouts[i].size; // No compression for v1
            tocEntry.PathHash          = entries[i].PathHash;

            memcpy(tocPtr, &tocEntry, sizeof(LPakTOCEntry));
            tocPtr += sizeof(LPakTOCEntry);

            memcpy(tocPtr, entries[i].Path.c_str(), entries[i].Path.size() + 1);
            tocPtr += entries[i].Path.size() + 1;
        }

        bool result = FileSystem::WriteFile(outputPath, buffer.data(), (u32)totalSize);

        LINFO("AssetPacker: Packed %zu files into %.*s (%llu bytes)",
              entries.size(), (int)outputPath.size, outputPath.str, (unsigned long long)totalSize);

        return result;
    }
}
