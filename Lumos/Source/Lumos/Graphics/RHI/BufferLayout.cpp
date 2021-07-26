#include "Precompiled.h"
#include "BufferLayout.h"

#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLTypes.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXTypes.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VK.h"
#endif

namespace Lumos
{
    namespace Graphics
    {
        BufferLayout::BufferLayout()
            : m_Size(0)
        {
        }

        void BufferLayout::Push(const std::string& name, Format format, uint32_t size, bool Normalised)
        {
            m_Layout.push_back({ name, format, m_Size, Normalised });
            m_Size += size;
        }

        template <>
        void BufferLayout::Push<uint32_t>(const std::string& name, bool Normalised)
        {
            Push(name, Format::R32_UINT, sizeof(uint32_t), Normalised);
        }

        template <>
        void BufferLayout::Push<uint8_t>(const std::string& name, bool Normalised)
        {
            Push(name, Format::R8_UINT, sizeof(uint8_t), Normalised);
        }

        template <>
        void BufferLayout::Push<float>(const std::string& name, bool Normalised)
        {
            Push(name, Format::R32_FLOAT, sizeof(float), Normalised);
        }

        template <>
        void BufferLayout::Push<Maths::Vector2>(const std::string& name, bool Normalised)
        {
            Push(name, Format::R32G32_FLOAT, sizeof(Maths::Vector2), Normalised);
        }

        template <>
        void BufferLayout::Push<Maths::Vector3>(const std::string& name, bool Normalised)
        {
            Push(name, Format::R32G32B32_FLOAT, sizeof(Maths::Vector3), Normalised);
        }

        template <>
        void BufferLayout::Push<Maths::Vector4>(const std::string& name, bool Normalised)
        {
            Push(name, Format::R32G32B32A32_FLOAT, sizeof(Maths::Vector4), Normalised);
        }
        template <>
        void BufferLayout::Push<Maths::IntVector3>(const std::string& name, bool Normalised)
        {
            Push(name, Format::R32G32B32_INT, sizeof(Maths::IntVector3), Normalised);
        }
        template <>
        void BufferLayout::Push<Maths::IntVector4>(const std::string& name, bool Normalised)
        {
            Push(name, Format::R32G32B32A32_INT, sizeof(Maths::IntVector4), Normalised);
        }
    }
}
