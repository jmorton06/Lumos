#include "Precompiled.h"
#include "RenderPass.h"

#include "Utilities/CombineHash.h"

namespace Lumos
{
    namespace Graphics
    {
        static std::unordered_map<std::size_t, SharedPtr<RenderPass>> m_RenderPassCache;

        RenderPass::~RenderPass() = default;
        RenderPass* (*RenderPass::CreateFunc)(const RenderPassDesc&) = nullptr;

        RenderPass* RenderPass::Create(const RenderPassDesc& renderPassDesc)
        {
            LUMOS_ASSERT(CreateFunc, "No RenderPass Create Function");

            return CreateFunc(renderPassDesc);
        }

        SharedPtr<RenderPass> RenderPass::Get(const RenderPassDesc& renderPassDesc)
        {
            size_t hash = 0;
            HashCombine(hash, renderPassDesc.attachmentCount, renderPassDesc.clear);

            for(int i = 0; i < renderPassDesc.attachmentCount; i++)
            {
                HashCombine(hash, renderPassDesc.attachmentTypes[i], renderPassDesc.attachments[i]);
            }

            auto found = m_RenderPassCache.find(hash);
            if(found != m_RenderPassCache.end() && found->second)
            {
                return found->second;
            }

            auto renderPass = SharedPtr<RenderPass>(Create(renderPassDesc));
            m_RenderPassCache[hash] = renderPass;
            return renderPass;
        }

        void RenderPass::ClearCache()
        {
            m_RenderPassCache.clear();
        }

        void RenderPass::DeleteUnusedCache()
        {
            static std::size_t keysToDelete[256];
            std::size_t keysToDeleteCount = 0;

            for(auto&& [key, value] : m_RenderPassCache)
            {
                if(value && value.GetCounter()->GetReferenceCount() == 1)
                {
                    keysToDelete[keysToDeleteCount] = key;
                    keysToDeleteCount++;
                }
            }

            for(std::size_t i = 0; i < keysToDeleteCount; i++)
            {
                m_RenderPassCache[keysToDelete[i]] = nullptr;
                m_RenderPassCache.erase(keysToDelete[i]);
            }
        }
    }
}
