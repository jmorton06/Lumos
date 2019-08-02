#include "LM.h"
#include "Core/OS.h"

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
