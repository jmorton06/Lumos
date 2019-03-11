#include "LM.h"
#include "BufferLayout.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLTypes.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXTypes.h"
#endif

namespace Lumos
{
	namespace graphics
	{
		BufferLayout::BufferLayout()
			: m_Size(0)
		{
		}

		void BufferLayout::Push(const String& name, uint type, uint size, uint count, bool normalized)
		{
			m_Layout.push_back({ name, type, size, count, m_Size, normalized });
			m_Size += size * count;
		}

		void BufferLayout::Push(const String& name, uint type, uint size, uint count, bool normalized, uint offset)
		{
			m_Layout.push_back({ name, type, size, count, m_Size, normalized });
			m_Size += offset;// size * count;
		}

        template<>
        void BufferLayout::Push<uint>(const String& name, uint count, bool normalized)
        {
            switch (graphics::Context::GetRenderAPI())
            {
#ifdef LUMOS_RENDER_API_OPENGL
                case RenderAPI::OPENGL:
                    Push(name, GL_UNSIGNED_INT, sizeof(uint), count, normalized);
                    break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
                case RenderAPI::DIRECT3D:
					Push(name, DX_TYPE_R32_UINT, sizeof(uint), count, normalized);
					break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
                case RenderAPI::VULKAN:
                    break;
#endif
            }
        }

        template<>
        void BufferLayout::Push<byte>(const String& name, uint count, bool normalized)
        {
            switch (graphics::Context::GetRenderAPI())
            {
#ifdef LUMOS_RENDER_API_OPENGL
                case RenderAPI::OPENGL:
                    Push(name, GL_UNSIGNED_BYTE, sizeof(byte), count, normalized);
                    break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
                case RenderAPI::DIRECT3D:
					Push(name, DX_TYPE_R8G8B8A8_UNORM, sizeof(byte) * 4, 1, normalized);
					break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
                case RenderAPI::VULKAN:
                    break;
#endif
            }
        }

		template<>
		void BufferLayout::Push<float>(const String& name, uint count, bool normalized)
		{
			switch (graphics::Context::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:
				Push(name, GL_FLOAT, sizeof(float), count, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:
				Push(name, DX_TYPE_R32_FLOAT, sizeof(float), count, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:
				break;
#endif
			}
		}

        template<>
        void BufferLayout::Push<maths::Vector2>(const String& name, uint count, bool normalized)
        {
            switch (graphics::Context::GetRenderAPI())
            {
#ifdef LUMOS_RENDER_API_OPENGL
                case RenderAPI::OPENGL:
                    Push(name, GL_FLOAT, sizeof(float), 2, normalized, sizeof(maths::Vector2));
                    break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
                case RenderAPI::DIRECT3D:
					Push(name, DX_TYPE_R32G32_FLOAT, sizeof(maths::Vector2), count, normalized);
					break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
                case RenderAPI::VULKAN:
                    break;
#endif
            }
        }

        template<>
        void BufferLayout::Push<maths::Vector3>(const String& name, uint count, bool normalized)
        {
            switch (graphics::Context::GetRenderAPI())
            {
#ifdef LUMOS_RENDER_API_OPENGL
                case RenderAPI::OPENGL:
                    Push(name, GL_FLOAT, sizeof(float), 3, normalized, sizeof(maths::Vector3));
                    break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
                case RenderAPI::DIRECT3D:
					Push(name, DX_TYPE_R32G32B32_FLOAT, sizeof(maths::Vector3), count, normalized);
					break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
                case RenderAPI::VULKAN:
                    break;
#endif
            }
        }

        template<>
        void BufferLayout::Push<maths::Vector4>(const String& name, uint count, bool normalized)
        {
            switch (graphics::Context::GetRenderAPI())
            {
#ifdef LUMOS_RENDER_API_OPENGL
                case RenderAPI::OPENGL:
                    Push(name, GL_FLOAT, sizeof(float), 4, normalized, sizeof(maths::Vector4));
                    break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
                case RenderAPI::DIRECT3D:
					Push(name, DX_TYPE_R32G32B32A32_FLOAT, sizeof(maths::Vector4), count, normalized);
					break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
                case RenderAPI::VULKAN:
                    break;
#endif
            }
        }

    }
}