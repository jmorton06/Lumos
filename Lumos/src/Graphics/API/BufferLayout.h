#pragma once

#include "LM.h"

#include "Maths/Maths.h"

namespace Lumos
{
	namespace Graphics
	{
		struct LUMOS_EXPORT BufferElement
		{
			String name;
			u32 type;
			u32 size;
			u32 count;
			u32 offset;
			bool normalized;
		};

		class LUMOS_EXPORT BufferLayout
		{
		private:
			u32 m_Size;
			std::vector<BufferElement> m_Layout;
		public:
			BufferLayout();

			template<typename T>
			void Push(const String& name, u32 count = 1, bool normalized = false)
			{
				LUMOS_CORE_ASSERT(false, "Unkown type!");
			}

			inline const std::vector<BufferElement>& GetLayout() const { return m_Layout; }
			inline u32 GetStride() const { return m_Size; }

		private:
			void Push(const String& name, u32 type, u32 size, u32 count, bool normalized);
			void Push(const String& name, u32 type, u32 size, u32 count, bool normalized, u32 offset);
		};

		template<>
		void LUMOS_EXPORT BufferLayout::Push<float>(const String& name, u32 count , bool normalized );
		template<>
		void LUMOS_EXPORT BufferLayout::Push<u32>(const String& name, u32 count , bool normalized );
		template<>
		void LUMOS_EXPORT BufferLayout::Push<u8>(const String& name, u32 count , bool normalized );
		template<>
		void LUMOS_EXPORT BufferLayout::Push<Maths::Vector2>(const String& name, u32 count , bool normalized );
		template<>
		void LUMOS_EXPORT BufferLayout::Push<Maths::Vector3>(const String& name, u32 count , bool normalized );
		template<>
		void LUMOS_EXPORT BufferLayout::Push<Maths::Vector4>(const String& name, u32 count , bool normalized );

	}
}