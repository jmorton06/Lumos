#include "Precompiled.h"
#include "WavWriter.h"
#include "Core/OS/FileSystem.h"
#include "Core/DataStructures/TDArray.h"

#include <cstring>

namespace Lumos
{
    bool WriteWav16(const std::string& path, const float* interleaved, u32 frames, u32 channels, u32 sampleRate)
    {
        if(!interleaved || channels == 0 || sampleRate == 0)
            return false;
        const u32 dataBytes = frames * channels * 2;
        TDArray<u8> out;
        out.Resize(44 + dataBytes);
        u8* p = out.Data();

        auto put32 = [&](u32 v) { memcpy(p, &v, 4); p += 4; };
        auto put16 = [&](u16 v) { memcpy(p, &v, 2); p += 2; };
        auto tag   = [&](const char* t) { memcpy(p, t, 4); p += 4; };

        tag("RIFF");
        put32(36 + dataBytes);
        tag("WAVE");
        tag("fmt ");
        put32(16);
        put16(1); // PCM
        put16((u16)channels);
        put32(sampleRate);
        put32(sampleRate * channels * 2);
        put16((u16)(channels * 2));
        put16(16);
        tag("data");
        put32(dataBytes);

        i16* s = (i16*)p;
        for(u32 i = 0; i < frames * channels; i++)
        {
            float v = interleaved[i];
            v       = v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
            s[i]    = (i16)(v * 32767.0f);
        }

        if(path.size() > 1 && path[0] == '/' && path[1] == '/')
            return FileSystem::Get().WriteFileVFS(Str8StdS(path), out.Data(), (u32)out.Size());
        return FileSystem::WriteFile(Str8StdS(path), out.Data(), (u32)out.Size());
    }
}
