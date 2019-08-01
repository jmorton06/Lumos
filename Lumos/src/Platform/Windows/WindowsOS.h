#include "LM.h"
#include "Core/OS.h"

namespace Lumos
{
    class WindowsOS : public OS
    {
	public:
        WindowsOS() {}
        ~WindowsOS() {}

        void Run() override {}
	};
}