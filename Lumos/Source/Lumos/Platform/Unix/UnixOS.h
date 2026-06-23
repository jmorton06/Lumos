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
        void SetWindowDecorations(bool decorated) override;
        void BeginWindowDrag() override;
        void UpdateWindowDrag() override;
        void BeginWindowResize(int edgeMask) override;
        void UpdateWindowResize() override;
        bool IsWindowMaximised() const override;
        void RestoreWindow() override;
        void IconifyWindow() override;

        std::string GetExecutablePath() override;
    };
}
