#include "Core/OS/OS.h"

namespace Lumos
{
    class MacOSOS : public OS
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
        void SetTitleBarColour(const glm::vec4& colour, bool dark = true) override;
        void Delay(uint32_t usec) override;
        void MaximiseWindow() override;
    };
}
