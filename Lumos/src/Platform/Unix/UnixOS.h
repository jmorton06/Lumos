#include "LM.h"
#include "Core/OS/OS.h"
#include "UnixThread.h"
#include "UnixMutex.h"

namespace Lumos
{
    class UnixOS : public OS
    {
    public:
        UnixOS() = default;
        ~UnixOS() = default;

        void Init();
        void Run() override;
    };
}
