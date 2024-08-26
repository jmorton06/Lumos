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

        void BufferLayout::Push(const std::string& name, RHIFormat format, uint32_t size, bool Normalised)
        {
            m_Layout.PushBack({ name, format, m_Size, Normalised });
            m_Size += size;
        }

        template <>
        void BufferLayout::Push<uint32_t>(const std::string& name, bool Normalised)
        {
            Push(name, RHIFormat::R32_UInt, sizeof(uint32_t), Normalised);
        }

        template <>
        void BufferLayout::Push<uint8_t>(const std::string& name, bool Normalised)
        {
            Push(name, RHIFormat::R8_UInt, sizeof(uint8_t), Normalised);
        }

        template <>
        void BufferLayout::Push<float>(const std::string& name, bool Normalised)
        {
            Push(name, RHIFormat::R32_Float, sizeof(float), Normalised);
        }

        template <>
        void BufferLayout::Push<Vec2>(const std::string& name, bool Normalised)
        {
            Push(name, RHIFormat::R32G32_Float, sizeof(Vec2), Normalised);
        }

        template <>
        void BufferLayout::Push<Vec3>(const std::string& name, bool Normalised)
        {
            Push(name, RHIFormat::R32G32B32_Float, sizeof(Vec3), Normalised);
        }

        template <>
        void BufferLayout::Push<Vec4>(const std::string& name, bool Normalised)
        {
            Push(name, RHIFormat::R32G32B32A32_Float, sizeof(Vec4), Normalised);
        }
    }
}
