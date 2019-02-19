#include "LM.h"
#include "OggLoader.h"

#include <stb/stb_vorbis.h>

namespace Lumos
{
	AudioData LoadOgg(const String& fileName)
	{
		AudioData data = AudioData();

		FILE*			m_FileHandle;
		stb_vorbis*		m_StreamHandle;
		stb_vorbis_info m_VorbisInfo;

		m_FileHandle = fopen(fileName.c_str(), "rb");

		if (!m_FileHandle)
		{
			LUMOS_CORE_ERROR("Failed to load OGG file '{0}'!", fileName);
		}

		m_StreamHandle = stb_vorbis_open_filename(fileName.c_str(), nullptr, nullptr);

		if (!m_StreamHandle)
		{
			LUMOS_CORE_ERROR("Failed to load OGG file '{0}'!", fileName);
		}

		// Get file info
		m_VorbisInfo = stb_vorbis_get_info(m_StreamHandle);
		data.Channels = m_VorbisInfo.channels;
		data.BitRate = 16;
		data.FreqRate = static_cast<float>(m_VorbisInfo.sample_rate);

		uint32 dataSize = stb_vorbis_stream_length_in_samples(m_StreamHandle) *  m_VorbisInfo.channels * sizeof(int16);
		int16* buffer = (int16*)malloc(dataSize * sizeof(uint16));
		stb_vorbis_get_samples_short_interleaved(m_StreamHandle, m_VorbisInfo.channels, (short *)buffer, dataSize);
		data.Data = (unsigned char*)buffer;
		data.Size = dataSize;

		data.Length = stb_vorbis_stream_length_in_seconds(m_StreamHandle) * 1000.0f;// * m_VorbisInfo.channels;

		stb_vorbis_close(m_StreamHandle);

		return data;
	}
}