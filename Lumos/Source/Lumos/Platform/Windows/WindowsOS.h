#pragma once
#include "Core/OS/OS.h"
#include "Maths/MathsFwd.h"

namespace Lumos
{
    class LUMOS_EXPORT WindowsOS : public OS
    {
    public:
        WindowsOS()  = default;
        ~WindowsOS() = default;

        void Init();
        void Run() override;
        std::string GetExecutablePath() override;

        void OpenFileLocation(const std::string& path) override;
        void OpenFileExternal(const std::string& path) override;
        void OpenURL(const std::string& url) override;

        void SetTitleBarColour(const Vec4& colour, bool dark = true) override;
    };
}
