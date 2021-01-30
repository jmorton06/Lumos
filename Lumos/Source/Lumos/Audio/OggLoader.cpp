#include "Precompiled.h"
#include "OggLoader.h"

#include <stb/stb_vorbis.h>

namespace Lumos
{
	AudioData LoadOgg(const std::string& fileName)
	{
		AudioData data = AudioData();

		const auto m_FileHandle = fopen(fileName.c_str(), "rb");

		if(!m_FileHandle)
		{
			LUMOS_LOG_CRITICAL("Failed to load OGG file '{0}'!", fileName);
		}

		auto m_StreamHandle = stb_vorbis_open_filename(fileName.c_str(), nullptr, nullptr);

		if(!m_StreamHandle)
		{
			LUMOS_LOG_CRITICAL("Failed to load OGG file '{0}'!", fileName);
		}

		// Get file info
		const stb_vorbis_info m_VorbisInfo = stb_vorbis_get_info(m_StreamHandle);
		data.Channels = m_VorbisInfo.channels;
		data.BitRate = 16;
		data.FreqRate = static_cast<float>(m_VorbisInfo.sample_rate);

		const uint32_t dataSize = stb_vorbis_stream_length_in_samples(m_StreamHandle) * m_VorbisInfo.channels * sizeof(int16_t);
		auto* buffer = static_cast<int16_t*>(malloc(dataSize * sizeof(uint16_t)));
		stb_vorbis_get_samples_short_interleaved(m_StreamHandle, m_VorbisInfo.channels, static_cast<short*>(buffer), dataSize);
		data.Data = reinterpret_cast<unsigned char*>(buffer);
		data.Size = dataSize;

		data.Length = stb_vorbis_stream_length_in_seconds(m_StreamHandle) * 1000.0f; // * m_VorbisInfo.channels;

		stb_vorbis_close(m_StreamHandle);

		fclose(m_FileHandle);

		return data;
	}
}
