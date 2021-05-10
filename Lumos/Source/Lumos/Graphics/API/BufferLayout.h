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
            uint32_t offset = 0;
            bool Normalised = false;
        };

        class LUMOS_EXPORT BufferLayout
        {
        private:
            uint32_t m_Size;
            std::vector<BufferElement> m_Layout;

        public:
            BufferLayout();

            template <typename T>
            void Push(const std::string& name, bool Normalised = false)
            {
                LUMOS_ASSERT(false, "Unkown type!");
            }

            inline const std::vector<BufferElement>& GetLayout() const
            {
                return m_Layout;
            }
            inline uint32_t GetStride() const
            {
                return m_Size;
            }

        private:
            void Push(const std::string& name, Format format, uint32_t size, bool Normalised);
        };

        template <>
        void LUMOS_EXPORT BufferLayout::Push<float>(const std::string& name, bool Normalised);
        template <>
        void LUMOS_EXPORT BufferLayout::Push<uint32_t>(const std::string& name, bool Normalised);
        template <>
        void LUMOS_EXPORT BufferLayout::Push<uint8_t>(const std::string& name, bool Normalised);
        template <>
        void LUMOS_EXPORT BufferLayout::Push<Maths::Vector2>(const std::string& name, bool Normalised);
        template <>
        void LUMOS_EXPORT BufferLayout::Push<Maths::Vector3>(const std::string& name, bool Normalised);
        template <>
        void LUMOS_EXPORT BufferLayout::Push<Maths::Vector4>(const std::string& name, bool Normalised);
        template <>
        void LUMOS_EXPORT BufferLayout::Push<Maths::IntVector3>(const std::string& name, bool Normalised);
        template <>
        void LUMOS_EXPORT BufferLayout::Push<Maths::IntVector4>(const std::string& name, bool Normalised);

    }
}
