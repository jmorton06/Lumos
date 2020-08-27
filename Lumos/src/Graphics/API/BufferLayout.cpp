#include "Precompiled.h"
#include "BufferLayout.h"

#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_OPENGL
#	include "Platform/OpenGL/GLTypes.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#	include "Graphics/DirectX/DXTypes.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
		BufferLayout::BufferLayout()
			: m_Size(0)
		{
		}

		void BufferLayout::Push(const std::string& name, u32 type, u32 size, u32 count, bool normalized)
		{
			m_Layout.push_back({name, type, size, count, m_Size, normalized});
			m_Size += size * count;
		}

		void BufferLayout::Push(const std::string& name, u32 type, u32 size, u32 count, bool normalized, u32 offset)
		{
			m_Layout.push_back({name, type, size, count, m_Size, normalized});
			m_Size += offset; // size * count;
		}

		template<>
		void BufferLayout::Push<u32>(const std::string& name, u32 count, bool normalized)
		{
			switch(Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:
				Push(name, GL_UNSIGNED_INT, sizeof(u32), count, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:
				Push(name, DX_TYPE_R32_UINT, sizeof(u32), count, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:
				break;
#endif
			default:
				break;
			}
		}

		template<>
		void BufferLayout::Push<u8>(const std::string& name, u32 count, bool normalized)
		{
			switch(Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:
				Push(name, GL_UNSIGNED_BYTE, sizeof(u8), count, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:
				Push(name, DX_TYPE_R8G8B8A8_UNORM, sizeof(u8) * 4, 1, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:
				break;
#endif
			default:
				break;
			}
		}

		template<>
		void BufferLayout::Push<float>(const std::string& name, u32 count, bool normalized)
		{
			switch(Graphics::GraphicsContext::GetRenderAPI())
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
			default:
				break;
			}
		}

		template<>
		void BufferLayout::Push<Maths::Vector2>(const std::string& name, u32 count, bool normalized)
		{
			switch(Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:
				Push(name, GL_FLOAT, sizeof(float), 2, normalized, sizeof(Maths::Vector2));
				break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:
				Push(name, DX_TYPE_R32G32_FLOAT, sizeof(Maths::Vector2), count, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:
				break;
#endif
			default:
				break;
			}
		}

		template<>
		void BufferLayout::Push<Maths::Vector3>(const std::string& name, u32 count, bool normalized)
		{
			switch(Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:
				Push(name, GL_FLOAT, sizeof(float), 3, normalized, sizeof(Maths::Vector3));
				break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:
				Push(name, DX_TYPE_R32G32B32_FLOAT, sizeof(Maths::Vector3), count, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:
				break;
#endif
			default:
				break;
			}
		}

		template<>
		void BufferLayout::Push<Maths::Vector4>(const std::string& name, u32 count, bool normalized)
		{
			switch(Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:
				Push(name, GL_FLOAT, sizeof(float), 4, normalized, sizeof(Maths::Vector4));
				break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:
				Push(name, DX_TYPE_R32G32B32A32_FLOAT, sizeof(Maths::Vector4), count, normalized);
				break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:
				break;
#endif
			default:
				break;
			}
		}

	}
}
