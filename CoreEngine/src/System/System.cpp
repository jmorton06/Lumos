#include "JM.h"
#include "System.h"
#include "VFS.h"


namespace jm
{ 
	namespace internal 
	{
	void System::Init()
	{
        JMLog::Init();
		JM_CORE_INFO("Initializing System");
		VFS::Init();
	}

	void System::Shutdown()
	{
		JM_CORE_INFO("Shutting down System");
		VFS::Shutdown();
    }
} }
