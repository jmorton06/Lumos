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

        ArenaTemp Scratch = ScratchBegin(0, 0);

        String8 physicalPath;
        if(!Lumos::FileSystem::Get().ResolvePhysicalPath(Scratch.arena, Str8StdS(fileName), &physicalPath))
        {
            LINFO("Failed to load Ogg file : File Not Found");
        }

        // TODO: Replace with filesystem call
        const auto m_FileHandle = fopen((const char*)physicalPath.str, "rb");

        if(!m_FileHandle)
        {
            LFATAL("Failed to load OGG file '%s'!", (const char*)physicalPath.str);
        }
        int error;
        auto m_StreamHandle = stb_vorbis_open_filename((const char*)physicalPath.str, &error, nullptr);

        if(!m_StreamHandle)
        {
            LFATAL("Failed to load OGG file '%s'! , Error %s", (const char*)physicalPath.str, error);
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

        fclose(m_FileHandle);

        return data;
    }
}
