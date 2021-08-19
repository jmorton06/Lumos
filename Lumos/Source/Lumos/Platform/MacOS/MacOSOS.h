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
        void SetTitleBarColour(const Maths::Vector4& colour, bool dark = true) override;
        void Delay(uint32_t usec) override;
    };
}
