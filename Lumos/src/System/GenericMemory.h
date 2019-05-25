#pragma once

#include "LM.h"

#define GENERIC_MEMORY_SMALL_MEMSWAP_MAX 16
namespace lumos
{

	struct GenericMemory
	{
		static inline void* memmove(void* dest, const void* src, uintptr amt)
		{
			return ::memmove(dest, src, amt);
		}

		static inline int32 memcmp(const void* dest, const void* src, uintptr amt)
		{
			return ::memcmp(dest, src, amt);
		}

		template<typename T>
		static inline void* memset(void* destIn, T val, uintptr amt)
		{
			T* dest = (T*)destIn;
			uintptr amtT = amt / sizeof(T);
			uintptr remainder = amt % sizeof(T);
			for (uintptr i = 0; i < amtT; ++i, ++dest) {
				memcpy(dest, &val, sizeof(T));
			}
			memcpy(dest, &val, remainder);
			return destIn;
		}

		static inline void* memzero(void* dest, uintptr amt)
		{
			return ::memset(dest, 0, amt);
		}

		static inline void* memcpy(void* dest, const void* src, uintptr amt)
		{
			return ::memcpy(dest, src, amt);
		}

		static void memswap(void* a, void* b, uintptr size)
		{
			if (size <= GENERIC_MEMORY_SMALL_MEMSWAP_MAX) {
				smallmemswap(a, b, size);
			}
			else {
				bigmemswap(a, b, size);
			}
		}

		template<typename T>
		static inline constexpr T align(const T ptr, uintptr alignment)
		{
			return (T)(((intptr)ptr + alignment - 1) & ~(alignment - 1));
		}

		static void* malloc(unsigned long long amt, uint32 alignment);
		static void* realloc(void* ptr, unsigned long long amt, uint32 alignment);
		static void* free(void* ptr);
		static unsigned long long getAllocSize(void* ptr);
	private:
		static void bigmemswap(void* a, void* b, unsigned long long size);
		static void smallmemswap(void* a, void* b, unsigned long long size)
		{
			LUMOS_CORE_ASSERT(size <= GENERIC_MEMORY_SMALL_MEMSWAP_MAX,"");
			char temp_data[GENERIC_MEMORY_SMALL_MEMSWAP_MAX];
			void* temp = static_cast<void*>(&temp_data);
			GenericMemory::memcpy(temp, a, size);
			GenericMemory::memcpy(a, b, size);
			GenericMemory::memcpy(b, temp, size);
		}
	};

	template<>
	inline void* GenericMemory::memset(void* dest, uint8 val, uintptr amt)
	{
		return ::memset(dest, val, amt);
	}
}
