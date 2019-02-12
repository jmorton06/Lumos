#pragma once

#include "LM.h"
#include "AudioData.h"

namespace Lumos
{
	class LUMOS_EXPORT Sound
	{
	public:
		static Sound*   Create(const String& fileName, const String& format);
		static Sound*	GetSound(const String& name);
		static bool		AddSound(const String& name, String& fileName);
		static void		DeleteSounds();

		unsigned char*	GetData() const { return m_Data.Data; }
		int				GetBitRate() const { return m_Data.BitRate; }
		float			GetFrequency() const { return m_Data.FreqRate; }
		int				GetChannels() const { return m_Data.Channels; }
		int				GetSize() const { return m_Data.Size; }
		//unsigned int	GetBuffer() const { return m_Buffer; }
		bool			IsStreaming() const { return m_Streaming; }
		double			GetLength() const;
		virtual double	StreamData(unsigned int	buffer, double timeLeft) { return 0.0f; }

	protected:

		Sound();
		virtual ~Sound();
		
		bool	m_Streaming;

		AudioData m_Data;

		static std::map<String, Sound*>* m_Sounds;
	};
}