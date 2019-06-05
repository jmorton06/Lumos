#include "LM.h"
#include "Query.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLQuery.h"
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/directx/DXQuery.h"
#endif

#include "Graphics/API/GraphicsContext.h"

namespace Lumos
{
	namespace Graphics
	{
		Query* Query::Create(QueryType type)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case Graphics::RenderAPI::OPENGL:		return new GLQuery(type);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case Graphics::RenderAPI::DIRECT3D:	return new DXQuery(type);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case Graphics::RenderAPI::VULKAN: UNIMPLEMENTED; return nullptr;
#endif
			}
			return nullptr;
		}
	}
}
