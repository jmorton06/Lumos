#include "lmpch.h"
#include "Core/OS/OS.h"

namespace Lumos
{
    class macOSOS : public OS
    {
    public:
        macOSOS() {}
        ~macOSOS() {}

        void Init();
        void Run() override;
        String GetExecutablePath() override;

	};
}
