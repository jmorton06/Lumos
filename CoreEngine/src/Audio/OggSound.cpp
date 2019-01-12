#include "JM.h"
#include "OggSound.h"

#include <AL/al.h>

namespace jm
{

#ifdef OGGVORBIS

	OggSound::OggSound()	
	{
		m_Streaming	= true;
		m_FileHandle	= nullptr;
	}

	OggSound::~OggSound()	
	{
		if (m_FileHandle)
		{
			ov_clear(&m_StreamHandle);
			fclose(m_FileHandle);
		}
	}

	void OggSound::LoadFromOgg(const String& filename)
	{
		m_FileHandle = fopen(filename.c_str(), "rb");

		if (!m_FileHandle)
		{
			JM_FATAL("Failed to load OGG file '" ,filename);
			return;
		}

		if (ov_open(m_FileHandle, &m_StreamHandle, nullptr, 0) < 0)
		{
			JM_FATAL("Failed to get OGG stream handle!");
			return;
		}

		vorbis_info* vorbisInfo = ov_info(&m_StreamHandle, -1);

		m_Channels	= vorbisInfo->channels;
		m_FreqRate	= (float)vorbisInfo->rate;
		//m_BitRate		= (unsigned int)vorbisInfo->bitrate_nominal / 10000;
		m_BitRate		= 16;	//For now!

		m_Length = (float)ov_time_total(&m_StreamHandle,-1) * 1000.0f;

		ov_time_seek(&m_StreamHandle, 0);
	}

	double OggSound::StreamData(ALuint	buffer, double timeLeft)
	{
		char	data[BUFFERLENGTH];
		int		read = 0;
		int		readResult = 0;
		int		section;

		int seek  = ov_time_seek(&m_StreamHandle, (m_Length - timeLeft) / 1000.0);

		if(seek != 0)
		{
			return StreamData(buffer,timeLeft + GetLength());
		}

		while(read < BUFFERLENGTH)
		{
			readResult = ov_read(&m_StreamHandle, data + read, BUFFERLENGTH - read, 0, 2, 1, & section);

			if(readResult > 0)
			{
				read += readResult;
			}
			else 
			{
				break;
			}
		}

		if(read > 0)
		{
			alBufferData(buffer, GetOALFormat(), data, read, (ALsizei)GetFrequency());
		}

		return (float)read / (m_Channels * m_FreqRate * (m_BitRate / 8.0)) * 1000.0f;
	}

#else

	OggSound::OggSound() : m_StreamHandle(nullptr)
	{
		m_Streaming = true;
		m_FileHandle = nullptr;
		m_VorbisInfo = stb_vorbis_info();
	}

	OggSound::~OggSound()
	{
		if (m_FileHandle)
		{
			stb_vorbis_close(m_StreamHandle);
			fclose(m_FileHandle);
		}
	}

	bool OggSound::LoadFromOgg(const String& filename)
	{
		m_FileHandle = fopen(filename.c_str(), "rb");

		if (!m_FileHandle)
		{
			JM_CORE_ERROR("Failed to load OGG file '{0}'!", filename);
			return false;
		}

		m_StreamHandle = stb_vorbis_open_filename(filename.c_str(), nullptr, nullptr);

		if (!m_StreamHandle)
		{
			JM_CORE_ERROR("Failed to load OGG file '{0}'!", filename);
			return false;
		}

		// Get file info
		m_VorbisInfo = stb_vorbis_get_info(m_StreamHandle);
		m_Channels = m_VorbisInfo.channels;
		m_BitRate = 16;
		m_FreqRate = static_cast<float>(m_VorbisInfo.sample_rate);

		if (m_Channels == 2) m_Format = AL_FORMAT_STEREO16;
		else			   m_Format = AL_FORMAT_MONO16;

		m_Length = stb_vorbis_stream_length_in_seconds(m_StreamHandle) * 1000.0f;// * m_VorbisInfo.channels;
		return true;
	}

	double OggSound::StreamData(ALuint buffer, double timeLeft)
	{
		std::unique_ptr<ALshort[]> data = std::make_unique<ALshort[]>(BUFFERLENGTH);
		int		read = 0;

		while (read < BUFFERLENGTH)
		{
            auto pointer = (data.get() + read);
			const int readResult = stb_vorbis_get_samples_short(m_StreamHandle, m_VorbisInfo.channels, &pointer, BUFFERLENGTH - read);

			if (readResult > 0)
				read += readResult*m_VorbisInfo.channels;
			else
				break;
		}

		if (read > 0)
		{
#ifdef JM_OPENAL
			alBufferData(buffer, GetOALFormat(), data.get(), read * sizeof(ALshort), static_cast<ALsizei>(GetFrequency()));
#endif
		}

		return (float)read / (m_Channels * m_FreqRate * (m_BitRate / 8.0)) * 1000.0f;
	}
#endif

}
