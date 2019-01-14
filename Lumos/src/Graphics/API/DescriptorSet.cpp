#include "JM.h"
#include "DescriptorSet.h"
#include "Context.h"

#ifdef JM_RENDER_API_VULKAN 
#include "Platform/GraphicsAPI/Vulkan/VKDescriptorSet.h"
#endif

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLDescriptorSet.h"
#endif

namespace jm
{
	namespace graphics
	{
		namespace api
		{
			DescriptorSet* DescriptorSet::Create(DescriptorInfo info)
			{
				switch (graphics::Context::GetRenderAPI())
				{
#ifdef JM_RENDER_API_OPENGL
				case RenderAPI::OPENGL:	return new GLDescriptorSet(info);
#endif
#ifdef JM_RENDER_API_VULKAN
				case RenderAPI::VULKAN:	return new graphics::VKDescriptorSet(info);
#endif
#ifdef JM_RENDER_API_DIRECT3D
				case RenderAPI::DIRECT3D: return nullptr;
#endif
				}
				return nullptr;
			}
		}
	}
}