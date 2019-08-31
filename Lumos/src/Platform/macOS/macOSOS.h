#include "LM.h"
#include "Core/OS/OS.h"

namespace Lumos
{
    class macOSOS : public OS
    {
    public:
        macOSOS() {}
        ~macOSOS() {}

        void Run() override;
	};
}
