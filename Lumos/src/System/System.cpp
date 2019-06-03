#include "LM.h"
#include "System.h"
#include "VFS.h"
#include "JobSystem.h"
#include "Profiler.h"
#include "Scripting/LuaScript.h"

namespace Lumos
{ 
	namespace Internal 
	{
	void CoreSystem::Init()
	{
		LUMOS_PROFILE(System::Profiler::SetEnabled(true));
		LUMOS_PROFILE(System::Profiler::OnInit());

		LMLog::OnInit();
		System::JobSystem::OnInit();
		LUMOS_CORE_INFO("Initializing System");
		VFS::OnInit();
        LuaScript::Instance()->OnInit();
	}

	void CoreSystem::Shutdown()
	{
		LUMOS_CORE_INFO("Shutting down System");
        LuaScript::Release();
		VFS::OnShutdown();
    }
} }
