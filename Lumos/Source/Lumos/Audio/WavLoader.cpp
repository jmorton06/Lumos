#include "Precompiled.h"
#include "WavLoader.h"

namespace Lumos
{
    AudioData LoadWav(const std::string& fileName)
    {
        AudioData data = AudioData();

        std::ifstream file(fileName.c_str(), std::ios::in | std::ios::binary);

        if(!file)
        {
            LUMOS_LOG_CRITICAL("Failed to load WAV file '{0}'!", fileName);
            return data;
        }

        std::string chunkName;
        uint32_t chunkSize = 0;

        while(!file.eof())
        {
            LoadWAVChunkInfo(file, chunkName, chunkSize);

            if(chunkName == "RIFF")
            {
                file.seekg(4, std::ios_base::cur);
            }
            else if(chunkName == "fmt ")
            {
                FMTCHUNK fmt {};

                file.read(reinterpret_cast<char*>(&fmt), sizeof(FMTCHUNK));

                data.BitRate = static_cast<uint32_t>(fmt.samp);
                data.FreqRate = static_cast<float>(fmt.srate);
                data.Channels = static_cast<uint32_t>(fmt.channels);
            }
            else if(chunkName == "data")
            {
                data.Size = chunkSize;
                data.Data = new unsigned char[data.Size];
                file.read(reinterpret_cast<char*>(data.Data), chunkSize);
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

        //Milliseconds
        data.Length = static_cast<float>(data.Size) / (data.Channels * data.FreqRate * (data.BitRate / 8.0f)) * 1000.0f;

        file.close();

        return data;
    }

    void LoadWAVChunkInfo(std::ifstream& file, std::string& name, unsigned int& size)
    {
        char chunk[4];
        file.read(reinterpret_cast<char*>(&chunk), 4);
        file.read(reinterpret_cast<char*>(&size), 4);

        name = std::string(chunk, 4);
    }
}
