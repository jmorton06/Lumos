#pragma once

#include "LM.h"

#include "Audio/Sound.h"

#include <AL/al.h>

namespace Lumos
{
	class ALSound : public Sound
	{
	public:
		ALSound(const String& fileName, const String& format);
		virtual ~ALSound();

		unsigned int GetBuffer() const { return m_Buffer; }
	private:
		static ALenum GetOALFormat(u32 bitRate, u32 channels);
		unsigned int m_Buffer;
		int	    m_Format;
	};
}