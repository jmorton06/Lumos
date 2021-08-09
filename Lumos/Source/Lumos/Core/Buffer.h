#pragma once

#include "Core.h"
#include "LMLog.h"

namespace Lumos {
	
	struct Buffer
	{
        uint8_t* Data;
		uint32_t Size;
		
		Buffer() : Data(nullptr), Size(0)
		{
		}
		
		Buffer(void* data, uint32_t size) : Data((uint8_t*)data), Size(size)
		{
		}
		
		static Buffer Copy(const void* data, uint32_t size)
		{
			Buffer buffer;
			buffer.Allocate(size);
			memcpy(buffer.Data, data, size);
			return buffer;
		}
		
		void Allocate(uint32_t size)
		{
			delete[] Data;
			Data = nullptr;
			
			if (size == 0)
				return;
			
			Data = new uint8_t[size];
			Size = size;
		}
		
		void Release()
		{
			delete[] Data;
			Data = nullptr;
			Size = 0;
		}
		
		void InitialiseEmpty()
		{
			if (Data)
				memset(Data, 0, Size);
		}
		
		template<typename T>
			T& Read(uint32_t offset = 0)
		{
			return *(T*)((uint8_t*)Data + offset);
		}
		
        uint8_t* ReadBytes(uint32_t size, uint32_t offset)
		{
			LUMOS_ASSERT(offset + size <= Size, "Index out of bounds");
            uint8_t* buffer = new uint8_t[size];
			memcpy(buffer, (uint8_t*)Data + offset, size);
			return buffer;
		}
		
		void Write(void* data, uint32_t size, uint32_t offset = 0)
		{
			LUMOS_ASSERT(offset + size <= Size, "Index out of bounds");
			memcpy((uint8_t*)Data + offset, data, size);
		}
		
		operator bool() const
		{
			return Data;
		}
		
        uint8_t& operator[](int index)
		{
			return ((uint8_t*)Data)[index];
		}
		
		uint8_t operator[](int index) const
		{
			return ((uint8_t*)Data)[index];
		}
		
		template<typename T>
			T* As()
		{
			return (T*)Data;
		}
		
		inline uint32_t GetSize() const { return Size; }
	};
	
}
