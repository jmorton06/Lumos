#include "Precompiled.h"
#include "MemoryManager.h"
#include "Utilities/StringUtilities.h"

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

        LINFO("Memory Info:");
        LINFO("\tPhysical Memory : %s / %s", apm.c_str(), tpm.c_str());
        LINFO("\tVirtual Memory : %s / %s: ", avm.c_str(), tvm.c_str());
    }
}
