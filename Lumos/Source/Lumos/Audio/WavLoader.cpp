#include "Precompiled.h"
#include "WavLoader.h"
#include "Core/OS/FileSystem.h"

namespace Lumos
{
    AudioData LoadWav(const std::string& fileName)
    {
        AudioData data = AudioData();

        String8 fileStr = Str8StdS(fileName);
        i64 fileSize    = FileSystem::Get().GetFileSizeVFS(fileStr);
        Arena* scratch  = ArenaAlloc(fileSize + Megabytes(1));
        u8* file        = fileSize > 0 ? FileSystem::Get().ReadFileVFS(scratch, fileStr) : nullptr;

        if(!file || fileSize < 12 || MemoryCompare(file, "RIFF", 4) != 0 || MemoryCompare(file + 8, "WAVE", 4) != 0)
        {
            LFATAL("Failed to load WAV file '%.*s'!", (int)fileStr.size, fileStr.str);
            ArenaRelease(scratch);
            return data;
        }

        auto read16 = [](const u8* p) { return (u32)p[0] | ((u32)p[1] << 8); };
        auto read32 = [](const u8* p) { return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); };

        i64 at = 12;
        while(at + 8 <= fileSize)
        {
            const u8* chunk = file + at;
            const u32 size  = read32(chunk + 4);
            const u8* body  = chunk + 8;

            if(MemoryCompare(chunk, "fmt ", 4) == 0 && at + 8 + 16 <= fileSize)
            {
                data.Channels = read16(body + 2);
                data.FreqRate = (float)read32(body + 4);
                data.BitRate  = read16(body + 14);
            }
            else if(MemoryCompare(chunk, "data", 4) == 0)
            {
                data.Size = (i64)size < fileSize - at - 8 ? size : (u32)(fileSize - at - 8);
                data.Data.Resize(data.Size);
                MemoryCopy(data.Data.Data(), body, data.Size);
                break;
            }
            at += 8 + (i64)size + (size & 1); // chunks are word aligned
        }

        // Milliseconds
        if(data.Channels > 0 && data.BitRate > 0 && data.FreqRate > 0.0f)
            data.Length = (float)data.Size / (data.Channels * data.FreqRate * (data.BitRate / 8.0f)) * 1000.0f;

        ArenaRelease(scratch);
        return data;
    }
}
