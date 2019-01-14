#pragma once

#include "LM.h"

namespace Lumos
{
	struct FMTCHUNK
	{
		short format;
		short channels;
		unsigned long srate;
		unsigned long bps;
		short balign;
		short samp;
	};

	struct RIFF_Header
	{
		char chunkID[4];
		long chunkSize;
		char format[4];
	};

	struct WAVE_Data
	{
		char subChunkID[4];
		long subChunk2Size;
	};

	struct WAVE_Format
	{
		char subChunkID[4];
		long subChunkSize;
		short audioFormat;
		short numChannels;
		long sampleRate;
		long byteRate;
		short blockAlign;
		short bitsPerSample;
	};

	class LUMOS_EXPORT Sound
	{
	public:
		unsigned char*	GetData() const { return m_Data; }
		int				GetBitRate() const { return m_BitRate; }
		float			GetFrequency() const { return m_FreqRate; }
		int				GetChannels() const { return m_Channels; }
		int				GetSize() const { return m_Size; }
		unsigned int	GetBuffer() const { return m_Buffer; }
		bool			IsStreaming() const { return m_Streaming; }
		virtual double	StreamData(unsigned int	buffer, double timeLeft){ return 0.0f; }

		int				GetOALFormat() const;
		double			GetLength() const;

		static bool		AddSound(const String& name, String& fileName);
		static Sound*	GetSound(const String& name);

		static void		DeleteSounds();

	protected:
		Sound();
		virtual ~Sound();

		bool LoadWAVFile(const String& filename);

		bool LoadFromWAV(const String& filename);
		static void LoadWAVChunkInfo(std::ifstream& file, String& name, uint& size);

		unsigned char*	m_Data;
		unsigned int	m_Buffer;
		int	    m_Format;
		bool	m_Streaming;

		float	m_FreqRate;
		double	m_Length;
		uint	m_BitRate;
		uint	m_Size;
		uint	m_Channels;

		static std::map<String, Sound*>* m_Sounds;
	};
}