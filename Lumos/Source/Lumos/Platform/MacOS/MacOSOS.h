#pragma once
#include "Platform/Unix/UnixOS.h"

namespace Lumos
{
    class MacOSOS : public UnixOS
    {
    public:
        MacOSOS()
        {
        }
        ~MacOSOS()
        {
        }

        void Init();
        void Run() override;
        std::string GetExecutablePath() override;
        void SetTitleBarColour(const Vec4& colour, bool dark = true) override;
        void SetWindowDecorations(bool decorated) override;
        void BeginWindowDrag() override;
        void UpdateWindowDrag() override;
        bool IsWindowMaximised() const override;
        bool IsWindowFullscreen() const override;
        void SetWindowFullscreen(bool fullscreen) override;
        bool DidEnterFullscreen() const override;
        void RestoreWindow() override;
        void Delay(uint32_t usec) override;
        void MaximiseWindow() override;
    };
}
