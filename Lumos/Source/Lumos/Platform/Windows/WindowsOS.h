
#include "Core/OS/OS.h"

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

        void OpenFileLocation(const std::filesystem::path& path) override;
        void OpenFileExternal(const std::filesystem::path& path) override;
        void OpenURL(const std::string& url) override;

        void SetTitleBarColour(const glm::vec4& colour, bool dark = true) override;
    };
}