#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Sound.h"
#include "Core/OS/FileSystem.h"

#ifdef LUMOS_OPENAL
#include "Platform/OpenAL/ALSound.h"
#endif

namespace Lumos
{
    Sound::Sound()
        : m_Streaming(false)
        , m_Data {}
    {
    }

    SharedPtr<Sound> Sound::Create(const std::string& name, const std::string& extension)
    {
#ifdef LUMOS_OPENAL
        return SharedPtr<ALSound>(new ALSound(name, extension));
#else
        return nullptr;
#endif
    }

    double Sound::GetLength() const
    {
        return m_Data.Length;
    }

    void Sound::ConvertToMono(const uint8_t* inputData, int dataSize, uint8_t* monoData, int channels, int bitsPerSample)
    {
        LUMOS_ASSERT(channels != 0, "0 Channels in audio file");
        if(channels == 1)
        {
            memcpy(monoData, inputData, dataSize);
            return;
        }
        uint32_t bytesPerSample = bitsPerSample / 8;

        uint32_t j           = 0;
        uint32_t channelSize = channels * bytesPerSample;
        for(uint32_t i = 0; i < uint32_t(dataSize); i += channelSize)
        {
            switch(bytesPerSample)
            {
            case 1:
            {
                uint64_t data = 0;
                for(uint32_t c = 0; c < channelSize; c += bytesPerSample)
                {
                    data += static_cast<uint64_t>(*(inputData + i + c));
                }
                monoData[j] = (uint8_t)(data / (uint64_t)channels);
                j += bytesPerSample;
            }
            break;
            case 2:
            {
                int64_t data = 0;
                for(uint32_t c = 0; c < channelSize; c += bytesPerSample)
                {
                    data += static_cast<const int64_t>(*reinterpret_cast<const int16_t*>(inputData + i + c));
                }
                *reinterpret_cast<int16_t*>(monoData + j) = (uint16_t)(data / (uint64_t)channels);
                j += bytesPerSample;
            }
            break;
            case 4:
            {
                float data = 0;
                for(uint32_t c = 0; c < channelSize; c += bytesPerSample)
                {
                    data += *reinterpret_cast<const float*>(inputData + i + c);
                }
                *reinterpret_cast<float*>(monoData + j) = data / channels;
                j += bytesPerSample;
            }
            break;
            default:
                LUMOS_LOG_WARN("Unsupported bytesPerSample");
                break;
            }
        }
    }
}
