#include "JM.h"
#include "Query.h"

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLQuery.h"
#endif

#ifdef JM_RENDER_API_DIRECT3D
#include "graphics/directx/DXQuery.h"
#endif

#include "Graphics/API/Context.h"

namespace jm
{

	Query* Query::Create(QueryType type)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLQuery(type);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new DXQuery(type);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN : UNIMPLEMENTED; return nullptr;
#endif
		}
		return nullptr;
	}
}
