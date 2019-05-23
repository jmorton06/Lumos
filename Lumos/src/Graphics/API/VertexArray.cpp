#include "LM.h"
#include "VertexArray.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLVertexArray.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXVertexArray.h"
#endif
#include "Graphics/API/GraphicsContext.h"
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKVertexArray.h"
#endif

namespace lumos
{
	namespace graphics
	{
		VertexArray* VertexArray::Create()
		{
			switch (GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLVertexArray();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKVertexArray();
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DVertexArray();
#endif
			}
			return nullptr;
		}

		void VertexArray::DeleteBuffers()
		{
			for (auto buffer : m_Buffers)
			{
				delete buffer;
				buffer = nullptr;
			}

			m_Buffers.clear();
		}
	}
}
