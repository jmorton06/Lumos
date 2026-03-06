#include "Precompiled.h"
#include "LImgReader.h"
#include "LImgFormat.h"
#include "Core/OS/FileSystem.h"

#include <lz4/lz4.h>

namespace Lumos
{
    bool LImgReader::Read(const char* physicalPath, LImgReadResult& outResult)
    {
        LUMOS_PROFILE_FUNCTION();

        FILE* f = fopen(physicalPath, "rb");
        if(!f)
            return false;

        LImgHeader header;
        if(fread(&header, sizeof(LImgHeader), 1, f) != 1)
        {
            fclose(f);
            return false;
        }

        if(memcmp(header.Magic, LIMG_MAGIC, 4) != 0 || header.Version != LIMG_VERSION)
        {
            fclose(f);
            return false;
        }

        u8* compressedBuf = new u8[header.CompressedSize];
        if(fread(compressedBuf, 1, header.CompressedSize, f) != header.CompressedSize)
        {
            delete[] compressedBuf;
            fclose(f);
            return false;
        }
        fclose(f);

        u8* pixels = new u8[header.UncompressedSize];
        int decompressed = LZ4_decompress_safe(
            (const char*)compressedBuf, (char*)pixels,
            (int)header.CompressedSize, (int)header.UncompressedSize);

        delete[] compressedBuf;

        if(decompressed != (int)header.UncompressedSize)
        {
            LERROR("LImgReader: LZ4 decompression failed for %s", physicalPath);
            delete[] pixels;
            return false;
        }

        outResult.Pixels = pixels;
        outResult.Width  = header.Width;
        outResult.Height = header.Height;
        outResult.IsHDR  = (header.Flags & LImgFlag_HDR) != 0;
        outResult.Bits   = outResult.IsHDR ? 128 : 32;

        return true;
    }
}
