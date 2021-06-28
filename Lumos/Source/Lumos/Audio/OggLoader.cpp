#include "Precompiled.h"
#include "OggLoader.h"
#include "Core/VFS.h"
#include "Sound.h"

#include <stb/stb_vorbis.h>

namespace Lumos
{
    AudioData LoadOgg(const std::string& fileName)
    {
        AudioData data = AudioData();

        std::string physicalPath;
        if(!Lumos::VFS::Get()->ResolvePhysicalPath(fileName, physicalPath))
        {
            LUMOS_LOG_INFO("Failed to load Ogg file : File Not Found");
        }

        const auto m_FileHandle = fopen(physicalPath.c_str(), "rb");

        if(!m_FileHandle)
        {
            LUMOS_LOG_CRITICAL("Failed to load OGG file '{0}'!", physicalPath);
        }
        int error;
        auto m_StreamHandle = stb_vorbis_open_filename(physicalPath.c_str(), &error, nullptr);

        if(!m_StreamHandle)
        {
            LUMOS_LOG_CRITICAL("Failed to load OGG file '{0}'! , Error {1}", physicalPath, error);
            return data;
        }

        // Get file info
        const stb_vorbis_info m_VorbisInfo = stb_vorbis_get_info(m_StreamHandle);
        data.Channels = m_VorbisInfo.channels;
        data.BitRate = 16;
        data.FreqRate = static_cast<float>(m_VorbisInfo.sample_rate);
        data.Size = stb_vorbis_stream_length_in_samples(m_StreamHandle) * m_VorbisInfo.channels * sizeof(int16_t);
        data.Data = new unsigned char[data.Size];

        stb_vorbis_get_samples_short_interleaved(m_StreamHandle, m_VorbisInfo.channels, reinterpret_cast<short*>(data.Data), data.Size);

        Sound::ConvertToMono(data.Data, data.Size, data.Data, data.Channels, data.BitRate);
        data.Channels = 1;
        data.Length = stb_vorbis_stream_length_in_seconds(m_StreamHandle) * 1000.0f; //Milliseconds

        stb_vorbis_close(m_StreamHandle);

        fclose(m_FileHandle);

        return data;
    }
}
