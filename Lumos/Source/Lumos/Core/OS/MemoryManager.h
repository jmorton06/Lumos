#pragma once
#include "Utilities/TSingleton.h"

namespace Lumos
{
    struct SystemMemoryInfo
    {
        int64_t availablePhysicalMemory;
        int64_t totalPhysicalMemory;

        int64_t availableVirtualMemory;
        int64_t totalVirtualMemory;

        void Log();
    };

    struct MemoryStats
    {
        int64_t totalAllocated;
        int64_t totalFreed;
        int64_t currentUsed;
        int64_t totalAllocations;

        MemoryStats()
            : totalAllocated(0)
            , totalFreed(0)
            , currentUsed(0)
            , totalAllocations(0)
        {
        }
    };

    class MemoryManager : public TSingleton<MemoryManager>
    {
        friend class TSingleton<MemoryManager>;

    public:
        MemoryStats m_MemoryStats;

    public:
        MemoryManager();

        void OnInit();
        inline MemoryStats GetMemoryStats() const
        {
            return m_MemoryStats;
        }

    public:
        SystemMemoryInfo GetSystemInfo();

    public:
        static std::string BytesToString(int64_t bytes);
    };
}
