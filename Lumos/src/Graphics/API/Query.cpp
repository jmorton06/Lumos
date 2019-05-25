#include "LM.h"
#include "Query.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLQuery.h"
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/directx/DXQuery.h"
#endif

#include "Graphics/API/GraphicsContext.h"

namespace lumos
{
	namespace graphics
	{
		Query* Query::Create(QueryType type)
		{
			switch (graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case graphics::RenderAPI::OPENGL:		return new GLQuery(type);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case graphics::RenderAPI::DIRECT3D:	return new DXQuery(type);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case graphics::RenderAPI::VULKAN: UNIMPLEMENTED; return nullptr;
#endif
			}
			return nullptr;
		}
	}
}
