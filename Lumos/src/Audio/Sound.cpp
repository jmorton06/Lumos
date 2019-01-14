#include "LM.h"
#include "Sound.h"
#include "OggSound.h"
#include "System/VFS.h"

#include <AL/al.h>

namespace Lumos
{

	std::map<String, Sound*>* Sound::m_Sounds = new std::map<String, Sound*>();

	Sound::Sound() : m_Format(0), m_Size(0), m_Channels(0)
	{
		m_Streaming = false;
		m_BitRate = 0;
		m_FreqRate = 0;
		m_Length = 0;
		m_Data = nullptr;
		m_Buffer = 0;
	}

	Sound::~Sound()
	{
		delete m_Data;
#ifdef LUMOS_OPENAL
		alDeleteBuffers(1, &m_Buffer);
#endif
	}

	double Sound::GetLength() const
	{
		return m_Length;
	}

	bool Sound::LoadFromWAV(const String& filename)
	{
		std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);

		if (!file)
		{
			LUMOS_CORE_ERROR("Failed to load WAV file '{0}'!", filename);
			return false;
		}

		std::string	 chunkName;
		uint chunkSize = 0;

		while (!file.eof())
		{
			LoadWAVChunkInfo(file, chunkName, chunkSize);

			if (chunkName == "RIFF")
			{
				file.seekg(4, std::ios_base::cur);
				//char waveString[4];
				//file.read((char*)&waveString,4);
			}
			else if (chunkName == "fmt ")
			{
				FMTCHUNK fmt{};

				file.read(reinterpret_cast<char*>(&fmt), sizeof(FMTCHUNK));

				m_BitRate  = static_cast<uint>(fmt.samp);
				m_FreqRate = static_cast<float>(fmt.srate);
				m_Channels = static_cast<uint>(fmt.channels);
			}
			else if (chunkName == "data")
			{
				m_Size = chunkSize;
				m_Data = new unsigned char[m_Size];
				file.read(reinterpret_cast<char*>(m_Data), chunkSize);
				break;
				/*
				In release mode, ifstream and / or something else were combining
				to make this function see another 'data' chunk, filled with
				nonsense data, breaking WAV loading. Easiest way to get around it
				was to simply break after loading the data chunk. This *should*
				be fine for any WAV file you find / use. Not fun to debug.
				*/
			}
			else
			{
				file.seekg(chunkSize, std::ios_base::cur);
			}
		}

		m_Length = static_cast<float>(m_Size) / (m_Channels * m_FreqRate * (m_BitRate / 8.0f)) * 1000.0f;

		file.close();
        return true;
	}

	void Sound::LoadWAVChunkInfo(std::ifstream &file, String &name, unsigned int &size)
	{
		char chunk[4];
		file.read(reinterpret_cast<char*>(&chunk), 4);
		file.read(reinterpret_cast<char*>(&size), 4);

		name = String(chunk, 4);
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

			if (extension == "wav")
			{
				s = new Sound();

				if (!s->LoadWAVFile(fileName))
					return false;
#ifdef LUMOS_OPENAL
				alGenBuffers(1, &s->m_Buffer);
				alBufferData(s->m_Buffer, s->GetOALFormat(), s->GetData(), s->GetSize(), static_cast<ALsizei>(s->GetFrequency()));
#endif
			}
			else if (extension == "ogg")
			{
				auto* ogg = new OggSound();
                if(!ogg->LoadFromOgg(fileName))
                    return false;
				s = ogg;
			}
			else
            {
				LUMOS_CORE_ERROR("Incompatible file extension '", extension);
                return false;
			}

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

	ALenum Sound::GetOALFormat() const
	{
		if (GetBitRate() == 16)
		{
			return GetChannels() == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
		}
		else if (GetBitRate() == 8)
		{
			return GetChannels() == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
		}
		return AL_FORMAT_MONO8;
	}

	bool Sound::LoadWAVFile(const String &filename)
	{
		FILE* soundFile = nullptr;
		WAVE_Format wave_format{};
		RIFF_Header riff_header{};
		WAVE_Data wave_data{};

		m_Size     = static_cast<uint>(wave_data.subChunk2Size);
		m_FreqRate = static_cast<float>(wave_format.sampleRate);

		if (wave_format.numChannels == 1)
		{
			if (wave_format.bitsPerSample == 8 )
				m_Format = AL_FORMAT_MONO8;
			else if (wave_format.bitsPerSample == 16)
				m_Format = AL_FORMAT_MONO16;
		}
		else if (wave_format.numChannels == 2)
		{
			if (wave_format.bitsPerSample == 8 )
				m_Format = AL_FORMAT_STEREO8;
			else if (wave_format.bitsPerSample == 16)
				m_Format = AL_FORMAT_STEREO16;
		}

		try {
            soundFile = fopen(filename.c_str(), "rb");
            if (!soundFile)
                throw (filename);

            // Read in the first chunk into the struct
            fread(&riff_header, sizeof(RIFF_Header), 1, soundFile);

            //check for RIFF and WAVE tag in memeory
            if ((riff_header.chunkID[0] != 'R' ||
                 riff_header.chunkID[1] != 'I' ||
                 riff_header.chunkID[2] != 'F' ||
                 riff_header.chunkID[3] != 'F') ||
                (riff_header.format[0] != 'W' ||
                 riff_header.format[1] != 'A' ||
                 riff_header.format[2] != 'V' ||
                 riff_header.format[3] != 'E'))
                throw ("Invalid RIFF or WAVE Header");

            //Read in the 2nd chunk for the wave info
            fread(&wave_format, sizeof(WAVE_Format), 1, soundFile);
            //check for fmt tag in memory
            if (wave_format.subChunkID[0] != 'f' ||
                wave_format.subChunkID[1] != 'm' ||
                wave_format.subChunkID[2] != 't' ||
                wave_format.subChunkID[3] != ' ')
                throw ("Invalid Wave Format");

            //check for extra parameters;
            if (wave_format.subChunkSize > 16)
                fseek(soundFile, sizeof(short), SEEK_CUR);

            //Read in the the last byte of data before the sound file
            fread(&wave_data, sizeof(WAVE_Data), 1, soundFile);

            //check for data tag in memory
            if (wave_data.subChunkID[0] != 'd' ||
                wave_data.subChunkID[1] != 'a' ||
                wave_data.subChunkID[2] != 't' ||
                wave_data.subChunkID[3] != 'a')
                throw ("Invalid data header");

            //Allocate memory for data
            m_Data = new unsigned char[wave_data.subChunk2Size];

            // Read in the sound data into the soundData variable
            if (!fread(m_Data, static_cast<size_t>(wave_data.subChunk2Size), 1, soundFile))
                throw ("error loading WAVE data into struct!");

            //Now we set the variables that we passed in with the
            //data from the structs
            m_Size     = static_cast<uint>(wave_data.subChunk2Size);
            m_FreqRate = static_cast<float>(wave_format.sampleRate);
            //The format is worked out by looking at the number of
            //channels and the bits per sample.
            if (wave_format.numChannels == 1)
            {
                if (wave_format.bitsPerSample == 8)
                    m_Format = AL_FORMAT_MONO8;
                else if (wave_format.bitsPerSample == 16)
                    m_Format = AL_FORMAT_MONO16;
            }
            else if (wave_format.numChannels == 2)
            {
                if (wave_format.bitsPerSample == 8)
                    m_Format = AL_FORMAT_STEREO8;
                else if (wave_format.bitsPerSample == 16)
                    m_Format = AL_FORMAT_STEREO16;
            }
            return true;
        }
        catch(const char* error)
        {
            //our catch statement for if we throw a string
            std::cerr << error << " : trying to load "
                      << filename << std::endl;
            //clean up memory if wave loading fails
            if (soundFile != nullptr)
                fclose(soundFile);
            //return false to indicate the failure to load wave
            return false;
        }
	}
}
