#include "LM.h"
#include "System.h"
#include "VFS.h"


namespace Lumos
{ 
	namespace internal 
	{
	void System::Init()
	{
        JMLog::Init();
		LUMOS_CORE_INFO("Initializing System");
		VFS::Init();
	}

	void System::Shutdown()
	{
		LUMOS_CORE_INFO("Shutting down System");
		VFS::Shutdown();
    }
} }
