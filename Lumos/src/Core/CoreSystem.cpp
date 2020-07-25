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
        Debug::Log::OnInit();

		if (enableProfiler)
		{
			Profiler::Get().Enable();
		}
		LUMOS_PROFILE_BLOCK("CoreSystem::Init");

		Debug::Log::Info("Lumos Engine - Version {0}.{1}.{2}", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);

		System::JobSystem::OnInit();
		Debug::Log::Info("Initializing System");
		VFS::OnInit();
        LuaManager::Get().OnInit();
	}

	void CoreSystem::Shutdown()
	{
		Debug::Log::Info("Shutting down System");
        Profiler::Release();
		LuaManager::Release();
		VFS::OnShutdown();
		Lumos::Memory::LogMemoryInformation();

		Debug::Log::OnRelease();

		MemoryManager::OnShutdown();
    }
} }
