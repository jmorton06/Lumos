#include "Precompiled.h"
#include "CoreSystem.h"
#include "VFS.h"
#include "JobSystem.h"
#include "Scripting/Lua/LuaManager.h"
#include "Core/Version.h"
 
#include "Core/OS/MemoryManager.h"

namespace Lumos
{ 
	namespace Internal 
	{
	void CoreSystem::Init(bool enableProfiler)
	{
        Debug::Log::OnInit();

		LUMOS_LOG_INFO("Lumos Engine - Version {0}.{1}.{2}", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);

		System::JobSystem::OnInit();
		LUMOS_LOG_INFO("Initializing System");
		VFS::OnInit();
        LuaManager::Get().OnInit();
	}

	void CoreSystem::Shutdown()
	{
		LUMOS_LOG_INFO("Shutting down System");
		LuaManager::Release();
		VFS::OnShutdown();
		Lumos::Memory::LogMemoryInformation();

		Debug::Log::OnRelease();

		MemoryManager::OnShutdown();
    }
} }
