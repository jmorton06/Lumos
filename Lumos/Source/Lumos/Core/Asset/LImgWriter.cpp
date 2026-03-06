#include "Precompiled.h"
#include "LImgWriter.h"
#include "LImgFormat.h"
#include "Core/OS/FileSystem.h"

#include <lz4/lz4.h>

namespace Lumos
{
    bool LImgWriter::Write(String8 path, u32 width, u32 height, const u8* pixels, Graphics::RHIFormat format)
    {
        u32 bpp = (format == Graphics::RHIFormat::R32G32B32A32_Float) ? 16 : 4;
        u32 uncompressedSize = width * height * bpp;

        int maxCompressed = LZ4_compressBound((int)uncompressedSize);
        u8* compressedBuf = new u8[maxCompressed];

        int compressedSize = LZ4_compress_default(
            (const char*)pixels, (char*)compressedBuf,
            (int)uncompressedSize, maxCompressed);

        if(compressedSize <= 0)
        {
            LERROR("LImgWriter: LZ4 compression failed for %s", (const char*)path.str);
            delete[] compressedBuf;
            return false;
        }

        LImgHeader header = {};
        memcpy(header.Magic, LIMG_MAGIC, 4);
        header.Version          = LIMG_VERSION;
        header.Flags            = LImgFlag_Compressed;
        header.Width            = width;
        header.Height           = height;
        header.Format           = (u32)format;
        header.CompressedSize   = (u32)compressedSize;
        header.UncompressedSize = uncompressedSize;

        if(bpp == 16)
            header.Flags |= LImgFlag_HDR;

        u64 totalSize = sizeof(LImgHeader) + compressedSize;
        u8* fileData  = new u8[totalSize];
        memcpy(fileData, &header, sizeof(LImgHeader));
        memcpy(fileData + sizeof(LImgHeader), compressedBuf, compressedSize);

        delete[] compressedBuf;

        bool result = FileSystem::WriteFile(path, fileData, (u32)totalSize);
        delete[] fileData;

        if(!result)
            LERROR("LImgWriter: Failed to write %s", (const char*)path.str);

        return result;
    }
}
