#include "LM.h"
#include "Core/OS.h"

namespace Lumos
{
    class iOSOS : public OS
    {
        iOSOS() {}
        ~iOSOS() {}

        void Run() override {}
    };
}