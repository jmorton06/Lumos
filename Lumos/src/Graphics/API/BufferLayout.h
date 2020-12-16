#pragma once

#include "Maths/Maths.h"
#include "DescriptorSet.h"

namespace Lumos
{
	namespace Graphics
	{
		struct LUMOS_EXPORT BufferElement
		{
			std::string name;
            Format format;
			u32 offset = 0;
			bool normalized = false;
		};

		class LUMOS_EXPORT BufferLayout
		{
		private:
			u32 m_Size;
			std::vector<BufferElement> m_Layout;

		public:
			BufferLayout();

			template<typename T>
			void Push(const std::string& name, bool normalized = false)
			{
				LUMOS_ASSERT(false, "Unkown type!");
			}

			_FORCE_INLINE_ const std::vector<BufferElement>& GetLayout() const
			{
				return m_Layout;
			}
			_FORCE_INLINE_ u32 GetStride() const
			{
				return m_Size;
			}

		private:
			void Push(const std::string& name, Format format, u32 size, bool normalized);
		};

		template<>
		void LUMOS_EXPORT BufferLayout::Push<float>(const std::string& name, bool normalized);
		template<>
		void LUMOS_EXPORT BufferLayout::Push<u32>(const std::string& name, bool normalized);
		template<>
		void LUMOS_EXPORT BufferLayout::Push<u8>(const std::string& name, bool normalized);
		template<>
		void LUMOS_EXPORT BufferLayout::Push<Maths::Vector2>(const std::string& name, bool normalized);
		template<>
		void LUMOS_EXPORT BufferLayout::Push<Maths::Vector3>(const std::string& name, bool normalized);
		template<>
		void LUMOS_EXPORT BufferLayout::Push<Maths::Vector4>(const std::string& name, bool normalized);
        template<>
        void LUMOS_EXPORT BufferLayout::Push<Maths::IntVector3>(const std::string& name, bool normalized);
        template<>
        void LUMOS_EXPORT BufferLayout::Push<Maths::IntVector4>(const std::string& name, bool normalized);

	}
}
