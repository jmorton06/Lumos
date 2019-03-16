#include "LM.h"
#include "Sound.h"
#include "System/VFS.h"

#ifdef LUMOS_OPENAL
#include "Platform/OpenAL/ALSound.h"
#endif

namespace Lumos
{
	std::map<String, Sound*>* Sound::m_Sounds = new std::map<String, Sound*>();
	Sound::Sound(): m_Streaming(false), m_Data(AudioData())
	{
	}

	Sound::~Sound()
	{
		delete[] m_Data.Data;
	}

	Sound* Sound::Create(const String& fileName, const String& format)
	{
#ifdef LUMOS_OPENAL
		return new ALSound(fileName, format);
#else
		return nullptr;
#endif
	}

	double Sound::GetLength() const
	{
		return m_Data.Length;
	}

	bool Sound::AddSound(const String& name, String& fileName)
	{
		Sound *s = GetSound(name);

		if (!s)
		{
			String physicalPath;
			if (!Lumos::VFS::Get()->ResolvePhysicalPath(fileName, physicalPath))
			{
				LUMOS_CORE_ERROR("Could not load Audio File : ", fileName);
			}

			fileName = physicalPath;

			std::string extension = fileName.substr(fileName.length() - 3, 3);

			s = Sound::Create(physicalPath, extension);
			m_Sounds->insert(make_pair(name, s));
			return true;
		}
		return true;
	}

	Sound* Sound::GetSound(const String& name)
	{
		const std::map<String, Sound*>::iterator s = m_Sounds->find(name);
		return (s != m_Sounds->end() ? s->second : nullptr);
	}

	void Sound::DeleteSounds()
	{
		for (auto & sound : *m_Sounds)
		{
			delete sound.second;
		}

		delete m_Sounds;
	}
}
