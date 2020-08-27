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
	};
}
