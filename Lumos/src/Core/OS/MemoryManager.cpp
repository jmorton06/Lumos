#include "lmpch.h"
#include "MemoryManager.h"
#include <iomanip>
namespace Lumos
{
	MemoryManager* MemoryManager::s_Instance = nullptr;

	MemoryManager::MemoryManager()
	{
	}

	void MemoryManager::OnInit()
	{
	}

	void MemoryManager::OnShutdown()
	{
		if(s_Instance)
			delete s_Instance;
	}

	MemoryManager* MemoryManager::Get()
	{
		if(s_Instance == nullptr)
		{
			s_Instance = new MemoryManager();
		}
		return s_Instance;
	}

	std::string MemoryManager::BytesToString(i64 bytes)
	{
		static const float gb = 1024 * 1024 * 1024;
		static const float mb = 1024 * 1024;
		static const float kb = 1024;

		std::stringstream result;
		if(bytes > gb)
			result << std::fixed << std::setprecision(2) << (float)bytes / gb << " gb";
		else if(bytes > mb)
			result << std::fixed << std::setprecision(2) << (float)bytes / mb << " mb";
		else if(bytes > kb)
			result << std::fixed << std::setprecision(2) << (float)bytes / kb << " kb";
		else
			result << std::fixed << std::setprecision(2) << (float)bytes << " bytes";

		return result.str();
	}

	void SystemMemoryInfo::Log()
	{
		std::string apm, tpm, avm, tvm;

		apm = MemoryManager::BytesToString(availablePhysicalMemory);
		tpm = MemoryManager::BytesToString(totalPhysicalMemory);
		avm = MemoryManager::BytesToString(availableVirtualMemory);
		tvm = MemoryManager::BytesToString(totalVirtualMemory);

		Debug::Log::Info("Memory Info:");
		Debug::Log::Info("\tPhysical Memory : {0} / {1}", apm, tpm);
		Debug::Log::Info("\tVirtual Memory : {0} / {1}: ", avm, tvm);
	}
}