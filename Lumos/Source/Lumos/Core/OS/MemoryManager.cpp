#include "Precompiled.h"
#include "MemoryManager.h"
#include "Utilities/StringUtilities.h"

namespace Lumos
{
    MemoryManager::MemoryManager()
    {
    }

    void MemoryManager::OnInit()
    {
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
