#include "JM.h"
#include "VertexArray.h"

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLVertexArray.h"
#endif
#ifdef JM_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXVertexArray.h"
#endif
#include "Graphics/API/Context.h"
#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKVertexArray.h"
#endif

namespace jm
{

	VertexArray* VertexArray::Create()
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLVertexArray();
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:		return new graphics::VKVertexArray();
#endif
#ifdef JM_RENDER_API_DIRECT3D
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
