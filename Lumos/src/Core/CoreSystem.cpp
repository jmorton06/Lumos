#include "lmpch.h"
#include "CoreSystem.h"
#include "VFS.h"
#include "JobSystem.h"
#include "Scripting/LuaManager.h"
#include "Core/Version.h"
#include "Core/Profiler.h"
#include "Core/OS/MemoryManager.h"

namespace Lumos
{ 
	namespace Internal 
	{
	void CoreSystem::Init(bool enableProfiler)
	{
        LMLog::OnInit();

		if (enableProfiler)
		{
			Profiler::Instance()->Enable();
			LUMOS_PROFILE_BLOCK("CoreSystem::Init");
		}

		LUMOS_LOG_INFO("Lumos Engine - Version {0}.{1}.{2}", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);

		System::JobSystem::OnInit();
		LUMOS_LOG_INFO("Initializing System");
		VFS::OnInit();
        LuaManager::Instance()->OnInit();
	}

	void CoreSystem::Shutdown()
	{
		LUMOS_LOG_INFO("Shutting down System");
        Profiler::Release();
		LuaManager::Release();
		VFS::OnShutdown();
		Lumos::Memory::LogMemoryInformation();

		LMLog::OnRelease();

		MemoryManager::OnShutdown();
    }
} }
