#include "Precompiled.h"
#include "Framebuffer.h"
#include "Texture.h"
#include "Graphics/RHI/GraphicsContext.h"

#include "Utilities/CombineHash.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VK.h"
#endif

namespace Lumos
{
    namespace Graphics
    {
        Framebuffer* (*Framebuffer::CreateFunc)(const FramebufferDesc&) = nullptr;

        Framebuffer* Framebuffer::Create(const FramebufferDesc& framebufferDesc)
        {
            LUMOS_ASSERT(CreateFunc, "No Framebuffer Create Function");

            return CreateFunc(framebufferDesc);
        }

        static std::unordered_map<std::size_t, SharedPtr<Framebuffer>> m_FramebufferCache;

        SharedPtr<Framebuffer> Framebuffer::Get(const FramebufferDesc& framebufferDesc)
        {
            size_t hash = 0;
            HashCombine(hash, framebufferDesc.attachmentCount, framebufferDesc.width, framebufferDesc.height, framebufferDesc.layer, framebufferDesc.renderPass, framebufferDesc.screenFBO);

            for(uint32_t i = 0; i < framebufferDesc.attachmentCount; i++)
            {
                HashCombine(hash, framebufferDesc.attachmentTypes[i], framebufferDesc.attachments[i]);
#ifdef LUMOS_RENDER_API_VULKAN

                if(GraphicsContext::GetRenderAPI() == RenderAPI::VULKAN)
                {
                    VkDescriptorImageInfo* imageHandle = (VkDescriptorImageInfo*)(framebufferDesc.attachments[i]->GetDescriptorInfo());
                    HashCombine(hash, imageHandle->imageLayout, imageHandle->imageView, imageHandle->sampler);
                }
#endif
            }

            auto found = m_FramebufferCache.find(hash);
            if(found != m_FramebufferCache.end() && found->second)
            {
                return found->second;
            }

            auto framebuffer         = SharedPtr<Framebuffer>(Create(framebufferDesc));
            m_FramebufferCache[hash] = framebuffer;
            return framebuffer;
        }

        void Framebuffer::ClearCache()
        {
            m_FramebufferCache.clear();
        }

        void Framebuffer::DeleteUnusedCache()
        {
            static std::size_t keysToDelete[256];
            std::size_t keysToDeleteCount = 0;

            for(auto&& [key, value] : m_FramebufferCache)
            {
                if(value && !value.GetCounter() && value.GetCounter()->GetReferenceCount() == 1)
                {
                    keysToDelete[keysToDeleteCount] = key;
                    keysToDeleteCount++;
                }
            }

            for(std::size_t i = 0; i < keysToDeleteCount; i++)
            {
                m_FramebufferCache[keysToDelete[i]] = nullptr;
                m_FramebufferCache.erase(keysToDelete[i]);
            }
        }

        Framebuffer::~Framebuffer()
        {
        }
    }
}
