#include "Precompiled.h"
#include "OggLoader.h"
#include "Core/OS/FileSystem.h"
#include "Sound.h"

#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

namespace Lumos
{
    AudioData LoadOgg(const std::string& fileName)
    {
        AudioData data = AudioData();

        String8 fileStr = Str8StdS(fileName);
        i64 fileSize    = FileSystem::Get().GetFileSizeVFS(fileStr);

        Arena* Scratch = ArenaAlloc(fileSize + Megabytes(1));

        u8* fileData    = fileSize > 0 ? FileSystem::Get().ReadFileVFS(Scratch, fileStr) : nullptr;

        if(!fileData)
        {
            LFATAL("Failed to load OGG file '%.*s' - not found (loose or packed)", (int)fileStr.size, fileStr.str);
            ArenaRelease(Scratch);
            return data;
        }

        int error;
        auto m_StreamHandle = stb_vorbis_open_memory((const unsigned char*)fileData, (int)fileSize, &error, nullptr);

        if(!m_StreamHandle)
        {
            LFATAL("Failed to load OGG file '%.*s'! , Error %i", (int)fileStr.size, fileStr.str, error);
            ArenaRelease(Scratch);
            return data;
        }

        // Get file info
        const stb_vorbis_info m_VorbisInfo = stb_vorbis_get_info(m_StreamHandle);
        data.Channels                      = m_VorbisInfo.channels;
        data.BitRate                       = 16;
        data.FreqRate                      = static_cast<float>(m_VorbisInfo.sample_rate);
        data.Size                          = stb_vorbis_stream_length_in_samples(m_StreamHandle) * m_VorbisInfo.channels * sizeof(int16_t);
        data.Data.Resize(data.Size);

        stb_vorbis_get_samples_short_interleaved(m_StreamHandle, m_VorbisInfo.channels, reinterpret_cast<short*>(data.Data.Data()), data.Size);

        Sound::ConvertToMono(data.Data.Data(), data.Size, data.Data.Data(), data.Channels, data.BitRate);
        data.Channels = 1;
        data.Length   = stb_vorbis_stream_length_in_seconds(m_StreamHandle) * 1000.0f; // Milliseconds

        stb_vorbis_close(m_StreamHandle);

        ArenaRelease(Scratch);

        return data;
    }
}
