#include "Precompiled.h"
#include "RenderPass.h"
#include "Core/Engine.h"
#include "Utilities/CombineHash.h"
#include "Graphics/RHI/Texture.h"

namespace Lumos
{
    namespace Graphics
    {
        struct RenderPassAsset
        {
            SharedPtr<RenderPass> renderpass;
            float timeSinceLastAccessed;
        };
        static std::unordered_map<uint64_t, RenderPassAsset> m_RenderPassCache;
        static const float m_CacheLifeTime = 0.1f;

        RenderPass::~RenderPass()                                    = default;
        RenderPass* (*RenderPass::CreateFunc)(const RenderPassDesc&) = nullptr;

        RenderPass* RenderPass::Create(const RenderPassDesc& renderPassDesc)
        {
            LUMOS_ASSERT(CreateFunc, "No RenderPass Create Function");

            return CreateFunc(renderPassDesc);
        }

        SharedPtr<RenderPass> RenderPass::Get(const RenderPassDesc& renderPassDesc)
        {
            LUMOS_PROFILE_FUNCTION();
            uint64_t hash = 0;
            HashCombine(hash, renderPassDesc.attachmentCount, renderPassDesc.clear);

            for(uint32_t i = 0; i < renderPassDesc.attachmentCount; i++)
            {
                HashCombine(hash, renderPassDesc.attachmentTypes[i], renderPassDesc.attachments[i], renderPassDesc.cubeMapIndex, renderPassDesc.mipIndex, renderPassDesc.samples);

				if(renderPassDesc.resolveTexture)
					HashCombine(hash, renderPassDesc.resolveTexture->GetUUID());

                if(renderPassDesc.attachments[i])
                    HashCombine(hash, renderPassDesc.attachments[i]->GetUUID());
            }

            auto found = m_RenderPassCache.find(hash);
            if(found != m_RenderPassCache.end() && found->second.renderpass)
            {
                found->second.timeSinceLastAccessed = (float)Engine::GetTimeStep().GetElapsedSeconds();
                return found->second.renderpass;
            }

            auto renderPass         = SharedPtr<RenderPass>(Create(renderPassDesc));
            m_RenderPassCache[hash] = { renderPass, (float)Engine::GetTimeStep().GetElapsedSeconds() };
            return renderPass;
        }

        void RenderPass::ClearCache()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPassCache.clear();
        }

        void RenderPass::DeleteUnusedCache()
        {
            LUMOS_PROFILE_FUNCTION();
            static std::size_t keysToDelete[256];
            std::size_t keysToDeleteCount = 0;

            for(auto&& [key, value] : m_RenderPassCache)
            {
                if(value.renderpass && value.renderpass.GetCounter() && value.renderpass.GetCounter()->GetReferenceCount() == 1 && (Engine::GetTimeStep().GetElapsedSeconds() - value.timeSinceLastAccessed) > m_CacheLifeTime)
                {
                    keysToDelete[keysToDeleteCount] = key;
                    keysToDeleteCount++;
                }

                if(keysToDeleteCount >= 256)
                    break;
            }

            for(std::size_t i = 0; i < keysToDeleteCount; i++)
            {
                m_RenderPassCache[keysToDelete[i]].renderpass = nullptr;
                m_RenderPassCache.erase(keysToDelete[i]);
            }
        }
        void RenderPass::BeginRenderPass(CommandBuffer* commandBuffer, float* clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const
        {
        }
    }
}
