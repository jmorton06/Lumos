#include "LM.h"
#include "Core/OS/OS.h"

namespace Lumos
{
    class UnixOS : public OS
    {
    public:
        UnixOS() {}
        ~UnixOS() {}

        void Run() override {}
    };
}