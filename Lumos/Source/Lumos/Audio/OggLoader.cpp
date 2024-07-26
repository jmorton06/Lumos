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

        std::string physicalPath;
        if(!Lumos::FileSystem::Get().ResolvePhysicalPath(fileName, physicalPath))
        {
            LINFO("Failed to load Ogg file : File Not Found");
        }

        const auto m_FileHandle = fopen(physicalPath.c_str(), "rb");

        if(!m_FileHandle)
        {
            LFATAL("Failed to load OGG file '{0}'!", physicalPath.c_str());
        }
        int error;
        auto m_StreamHandle = stb_vorbis_open_filename(physicalPath.c_str(), &error, nullptr);

        if(!m_StreamHandle)
        {
            LFATAL("Failed to load OGG file '{0}'! , Error {1}", physicalPath.c_str(), error);
            return data;
        }

        // Get file info
        const stb_vorbis_info m_VorbisInfo = stb_vorbis_get_info(m_StreamHandle);
        data.Channels                      = m_VorbisInfo.channels;
        data.BitRate                       = 16;
        data.FreqRate                      = static_cast<float>(m_VorbisInfo.sample_rate);
        data.Size                          = stb_vorbis_stream_length_in_samples(m_StreamHandle) * m_VorbisInfo.channels * sizeof(int16_t);
        data.Data.resize(data.Size);

        stb_vorbis_get_samples_short_interleaved(m_StreamHandle, m_VorbisInfo.channels, reinterpret_cast<short*>(data.Data.data()), data.Size);

        Sound::ConvertToMono(data.Data.data(), data.Size, data.Data.data(), data.Channels, data.BitRate);
        data.Channels = 1;
        data.Length   = stb_vorbis_stream_length_in_seconds(m_StreamHandle) * 1000.0f; // Milliseconds

        stb_vorbis_close(m_StreamHandle);

        fclose(m_FileHandle);

        return data;
    }
}
