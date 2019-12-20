#include "lmpch.h"
#include "Sound.h"
#include "Core/VFS.h"

#ifdef LUMOS_OPENAL
#include "Platform/OpenAL/ALSound.h"
#endif

namespace Lumos
{
	Sound::Sound(): m_Streaming(false), m_Data(AudioData())
	{
	}

	Sound::~Sound()
	{
		delete[] m_Data.Data;
	}

	Sound* Sound::Create(const String& name, const String& extension)
	{
#ifdef LUMOS_OPENAL
		return lmnew ALSound(name, extension);
#else
		return nullptr;
#endif
	}

	double Sound::GetLength() const
	{
		return m_Data.Length;
	}
}
