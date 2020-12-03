#include "Precompiled.h"
#include "RenderPass.h"

#include "Utilities/CombineHash.h"

namespace Lumos
{
	namespace Graphics
	{
        static std::unordered_map<std::size_t, Ref<RenderPass>> m_RenderPassCache;

		RenderPass::~RenderPass() = default;
        RenderPass*(*RenderPass::CreateFunc)(const RenderPassInfo&) = nullptr;

		RenderPass* RenderPass::Create(const RenderPassInfo& renderPassCI)
		{
            LUMOS_ASSERT(CreateFunc, "No RenderPass Create Function");
            
            return CreateFunc(renderPassCI);
		}
    
        Ref<RenderPass> RenderPass::Get(const RenderPassInfo& renderPassInfo)
        {
            size_t hash = 0;
            HashCombine(hash, renderPassInfo.attachmentCount, renderPassInfo.clear);
            
            for(int i = 0; i < renderPassInfo.attachmentCount; i++)
            {
                HashCombine(hash, renderPassInfo.textureType[i].format, renderPassInfo.textureType[i].textureType);
            }
            
            auto found = m_RenderPassCache.find(hash);
            if (found != m_RenderPassCache.end())
            {
                return found->second;
            }
            
            auto renderpass = Ref<RenderPass>(Create(renderPassInfo));
            m_RenderPassCache[hash] = renderpass;
            return renderpass;
        }
    
        void RenderPass::ClearCache()
        {
            m_RenderPassCache.clear();
        }
	}
}
