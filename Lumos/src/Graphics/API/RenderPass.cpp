#include "Precompiled.h"
#include "RenderPass.h"

namespace Lumos
{
	namespace Graphics
	{
		RenderPass::~RenderPass() = default;
        RenderPass*(*RenderPass::CreateFunc)(const RenderPassInfo&) = nullptr;

		RenderPass* RenderPass::Create(const RenderPassInfo& renderPassCI)
		{
            LUMOS_ASSERT(CreateFunc, "No RenderPass Create Function");
            
            return CreateFunc(renderPassCI);
		}
	}
}
