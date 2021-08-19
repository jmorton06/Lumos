#pragma once
#include "Core/OS/OS.h"

namespace Lumos
{
    class UnixOS : public OS
    {
    public:
        UnixOS() = default;
        ~UnixOS() = default;

        void Init();
        void Run() override;
        void Delay(uint32_t usec) override;

        std::string GetExecutablePath() override
        {
            return "";
        }
    };
}
