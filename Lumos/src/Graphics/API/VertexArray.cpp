#include "LM.h"
#include "VertexArray.h"
#include "VertexBuffer.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLVertexArray.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXVertexArray.h"
#endif
#include "Graphics/API/GraphicsContext.h"
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKVertexArray.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
		VertexArray* VertexArray::Create()
		{
			switch (GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return lmnew GLVertexArray();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return lmnew VKVertexArray();
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return lmnew D3DVertexArray();
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
