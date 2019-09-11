#pragma once

#include "lmpch.h"
#include "AudioData.h"
#include "Utilities/TSingleton.h"

namespace Lumos
{
	class LUMOS_EXPORT Sound
	{
		friend class SoundManager;
	public:
		static Sound*   Create(const String& fileName, const String& format);

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
	};

	class SoundManager : public TSingleton<SoundManager>
	{
		friend class TSingleton<SoundManager>;
		friend class Sound;
	public:
		SoundManager() = default;
		~SoundManager();

		Sound*	GetSound(const String& name);
		bool	AddSound(const String& name, String& fileName);

	private:
		std::map<String, Sound*> m_Sounds;
	};

}