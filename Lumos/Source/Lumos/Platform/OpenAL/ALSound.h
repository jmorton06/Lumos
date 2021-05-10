#pragma once

#include "Audio/Sound.h"

#include <AL/al.h>

namespace Lumos
{
    class ALSound : public Sound
    {
    public:
        ALSound(const std::string& fileName, const std::string& format);
        virtual ~ALSound();

        unsigned int GetBuffer() const
        {
            return m_Buffer;
        }

    private:
        static ALenum GetOALFormat(uint32_t bitRate, uint32_t channels);
        unsigned int m_Buffer;
        int m_Format;
    };
}
