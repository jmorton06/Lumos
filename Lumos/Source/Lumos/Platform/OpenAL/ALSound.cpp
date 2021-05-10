#include "Precompiled.h"
#include "ALSound.h"

#include "Audio/WavLoader.h"
#include "Audio/OggLoader.h"

namespace Lumos
{
    ALSound::ALSound(const std::string& fileName, const std::string& format)
        : m_Format(0)
    {
        m_FilePath = fileName;
        if(format == "wav")
            m_Data = LoadWav(fileName);
        else if(format == "ogg")
            m_Data = LoadOgg(fileName);

        alGenBuffers(1, &m_Buffer);
        alBufferData(m_Buffer, GetOALFormat(m_Data.BitRate, m_Data.Channels), m_Data.Data, m_Data.Size, static_cast<ALsizei>(m_Data.FreqRate));
    }

    ALSound::~ALSound()
    {
        alDeleteBuffers(1, &m_Buffer);
    }

    ALenum ALSound::GetOALFormat(uint32_t bitRate, uint32_t channels)
    {
        if(bitRate == 16)
        {
            return channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        }
        else if(bitRate == 8)
        {
            return channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
        }
        return AL_FORMAT_MONO8;
    }
}
