#pragma once

#include "LM.h"

#include "Maths/Maths.h"
#include "Context.h"

namespace Lumos
{
	namespace graphics
	{
		struct LUMOS_EXPORT BufferElement
		{
			String name;
			uint type;
			uint size;
			uint count;
			uint offset;
			bool normalized;
		};

		class LUMOS_EXPORT BufferLayout
		{
		private:
			uint m_Size;
			std::vector<BufferElement> m_Layout;
		public:
			BufferLayout();

			template<typename T>
			void Push(const String& name, uint count = 1, bool normalized = false)
			{
				LUMOS_CORE_ASSERT(false, "Unkown type!");
			}

			inline const std::vector<BufferElement>& GetLayout() const { return m_Layout; }
			inline uint GetStride() const { return m_Size; }

		private:
			void Push(const String& name, uint type, uint size, uint count, bool normalized);
			void Push(const String& name, uint type, uint size, uint count, bool normalized, uint offset);
		};

		template<>
		void BufferLayout::Push<float>(const String& name, uint count , bool normalized );
		template<>
		void BufferLayout::Push<uint>(const String& name, uint count , bool normalized );
		template<>
		void BufferLayout::Push<byte>(const String& name, uint count , bool normalized );
		template<>
		void BufferLayout::Push<maths::Vector2>(const String& name, uint count , bool normalized );
		template<>
		void BufferLayout::Push<maths::Vector3>(const String& name, uint count , bool normalized );
		template<>
		void BufferLayout::Push<maths::Vector4>(const String& name, uint count , bool normalized );

	}
}