#include "Precompiled.h"
#include "MemoryManager.h"
#include "Core/StringUtilities.h"
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

    void SystemMemoryInfo::Log()
    {
        std::string apm, tpm, avm, tvm;

        apm = StringUtilities::BytesToString(availablePhysicalMemory);
        tpm = StringUtilities::BytesToString(totalPhysicalMemory);
        avm = StringUtilities::BytesToString(availableVirtualMemory);
        tvm = StringUtilities::BytesToString(totalVirtualMemory);

        LUMOS_LOG_INFO("Memory Info:");
        LUMOS_LOG_INFO("\tPhysical Memory : {0} / {1}", apm, tpm);
        LUMOS_LOG_INFO("\tVirtual Memory : {0} / {1}: ", avm, tvm);
    }
}
