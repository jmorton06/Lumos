#include "Precompiled.h"
#include "RenderPass.h"

#include "Utilities/CombineHash.h"

namespace Lumos
{
    namespace Graphics
    {
        static std::unordered_map<std::size_t, SharedRef<RenderPass>> m_RenderPassCache;

        RenderPass::~RenderPass() = default;
        RenderPass* (*RenderPass::CreateFunc)(const RenderPassDesc&) = nullptr;

        RenderPass* RenderPass::Create(const RenderPassDesc& renderPassCI)
        {
            LUMOS_ASSERT(CreateFunc, "No RenderPass Create Function");

            return CreateFunc(renderPassCI);
        }

        SharedRef<RenderPass> RenderPass::Get(const RenderPassDesc& renderPassInfo)
        {
            size_t hash = 0;
            HashCombine(hash, renderPassInfo.attachmentCount, renderPassInfo.clear);

            for(int i = 0; i < renderPassInfo.attachmentCount; i++)
            {
                HashCombine(hash, renderPassInfo.textureType[i].format, renderPassInfo.textureType[i].textureType);
            }

            auto found = m_RenderPassCache.find(hash);
            if(found != m_RenderPassCache.end() && found->second)
            {
                return found->second;
            }

            auto renderpass = SharedRef<RenderPass>(Create(renderPassInfo));
            m_RenderPassCache[hash] = renderpass;
            return renderpass;
        }

        void RenderPass::ClearCache()
        {
            m_RenderPassCache.clear();
        }

        void RenderPass::DeleteUnusedCache()
        {
            for(const auto& [key, value] : m_RenderPassCache)
            {
                if(value && value.GetCounter()->GetReferenceCount() == 1)
                    m_RenderPassCache[key] = nullptr;
            }
        }
    }
}
