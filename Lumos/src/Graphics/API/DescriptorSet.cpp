#include "LM.h"
#include "DescriptorSet.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN 
#include "Platform/Vulkan/VKDescriptorSet.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLDescriptorSet.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
		DescriptorSet* DescriptorSet::Create(DescriptorInfo info)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:	return new GLDescriptorSet(info);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new VKDescriptorSet(info);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D: return nullptr;
#endif
			}
			return nullptr;
		}
	}
}