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

        void OpenFileLocation(const std::filesystem::path& path) override;
        void OpenFileExternal(const std::filesystem::path& path) override;
        void OpenURL(const std::string& url) override;

        std::string GetExecutablePath() override
        {
            return "";
        }
    };
}
