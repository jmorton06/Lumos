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

	Sound* Sound::Create(const String& name, const String& fileName)
	{
#ifdef LUMOS_OPENAL
		std::string extension = fileName.substr(fileName.length() - 3, 3);
		return lmnew ALSound(fileName, extension);
#else
		return nullptr;
#endif
	}

	double Sound::GetLength() const
	{
		return m_Data.Length;
	}
}
