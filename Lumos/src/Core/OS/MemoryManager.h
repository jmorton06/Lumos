#pragma once

#include "lmpch.h"

namespace Lumos
{
		struct SystemMemoryInfo
		{
			i64 availablePhysicalMemory;
			i64 totalPhysicalMemory;

			i64 availableVirtualMemory;
			i64 totalVirtualMemory;

			void Log();
		};

		struct MemoryStats
		{
			i64 totalAllocated;
			i64 totalFreed;
			i64 currentUsed;
			i64 totalAllocations;

			MemoryStats()
				: totalAllocated(0), totalFreed(0), currentUsed(0), totalAllocations(0)
			{
			}
		};

		class MemoryManager
		{
		public:
			static MemoryManager* s_Instance;
		public:
			MemoryStats m_MemoryStats;
		public:
			MemoryManager();

			static void OnInit();
			static void OnShutdown();

			static MemoryManager* Get();
			inline MemoryStats GetMemoryStats() const { return m_MemoryStats; }
		public:
			SystemMemoryInfo GetSystemInfo();
		public:
			static String BytesToString(i64 bytes);
		};
}