#include "LM.h"
#include "DescriptorSet.h"
#include "Context.h"

#ifdef LUMOS_RENDER_API_VULKAN 
#include "Platform/Vulkan/VKDescriptorSet.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLDescriptorSet.h"
#endif

namespace Lumos
{
	namespace graphics
	{
		namespace api
		{
			DescriptorSet* DescriptorSet::Create(DescriptorInfo info)
			{
				switch (graphics::Context::GetRenderAPI())
				{
#ifdef LUMOS_RENDER_API_OPENGL
				case RenderAPI::OPENGL:	return new GLDescriptorSet(info);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
				case RenderAPI::VULKAN:	return new graphics::VKDescriptorSet(info);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
				case RenderAPI::DIRECT3D: return nullptr;
#endif
				}
				return nullptr;
			}
		}
	}
}