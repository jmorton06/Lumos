#pragma once
#include "AudioData.h"

namespace Lumos
{
    struct FMTCHUNK
    {
        short format;
        short channels;
        unsigned long srate;
        unsigned long bps;
        short balign;
        short samp;
    };

    struct RIFF_Header
    {
        char chunkID[4];
        long chunkSize;
        char format[4];
    };

    struct WAVE_Data
    {
        char subChunkID[4];
        long subChunk2Size;
    };

    struct WAVE_Format
    {
        char subChunkID[4];
        long subChunkSize;
        short audioFormat;
        short numChannels;
        long sampleRate;
        long byteRate;
        short blockAlign;
        short bitsPerSample;
    };

    AudioData LoadWav(const std::string& fileName);

    void LoadWAVChunkInfo(std::ifstream& file, std::string& name, unsigned int& size);

}