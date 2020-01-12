#include "lmpch.h"
#include "Core/OS/OS.h"

namespace Lumos
{
    class LUMOS_EXPORT WindowsOS : public OS
    {
	public:
        WindowsOS() = default;
        ~WindowsOS() = default;

        void Init();
        void Run() override;
        const char* GetExecutablePath() override { return nullptr; }

	};
}