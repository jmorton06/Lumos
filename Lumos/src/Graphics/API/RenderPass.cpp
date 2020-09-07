#include "Precompiled.h"
#include "RenderPass.h"

namespace Lumos
{
	namespace Graphics
	{
		RenderPass::~RenderPass() = default;
        RenderPass*(*RenderPass::CreateFunc)() = nullptr;

		RenderPass* RenderPass::Create()
		{
            LUMOS_ASSERT(CreateFunc, "No RenderPass Create Function");
            
            return CreateFunc();
		}
	}
}
