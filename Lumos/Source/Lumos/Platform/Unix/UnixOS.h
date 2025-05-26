#pragma once
#include "Core/OS/OS.h"

namespace Lumos
{
    class UnixOS : public OS
    {
    public:
        UnixOS()  = default;
        ~UnixOS() = default;

        void Init();
        void Run() override;
        void Delay(uint32_t usec) override;

        void OpenFileLocation(const std::string& path) override;
        void OpenFileExternal(const std::string& path) override;
        void OpenURL(const std::string& url) override;

        std::string GetExecutablePath() override;
    };
}
