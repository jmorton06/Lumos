#include "JM.h"
#include "System.h"
#include "VFS.h"


namespace jm
{ 
	namespace internal 
	{
	void System::Init()
	{
		JM_INFO("Initializing System");
		VFS::Init();
	}

	void System::Shutdown()
	{
		JM_INFO("Shutting down System");
		VFS::Shutdown();
    }
} }
