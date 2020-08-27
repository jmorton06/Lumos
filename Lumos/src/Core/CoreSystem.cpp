#include "Precompiled.h"
#include "CoreSystem.h"
#include "VFS.h"
#include "JobSystem.h"
#include "Scripting/Lua/LuaManager.h"
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

		Debug::Log::Info("Lumos Engine - Version {0}.{1}.{2}", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);

		System::JobSystem::OnInit();
		Debug::Log::Info("Initializing System");
		VFS::OnInit();
        LuaManager::Get().OnInit();
	}

	void CoreSystem::Shutdown()
	{
		Debug::Log::Info("Shutting down System");
		LuaManager::Release();
		VFS::OnShutdown();
		Lumos::Memory::LogMemoryInformation();

		Debug::Log::OnRelease();

		MemoryManager::OnShutdown();
    }
} }
