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
                HashCombine(hash, framebufferDesc.attachmentTypes[i]);

                if(framebufferDesc.attachments[i])
                    HashCombine(hash, framebufferDesc.attachments[i]->GetImageHande());
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
            static const size_t keyDeleteSize = 256;
            static std::size_t keysToDelete[keyDeleteSize];
            std::size_t keysToDeleteCount = 0;

            for(auto&& [key, value] : m_FramebufferCache)
            {
                if(!value)
                {
                    keysToDelete[keysToDeleteCount] = key;
                    keysToDeleteCount++;
                }
                else if(value.GetCounter() && value.GetCounter()->GetReferenceCount() == 1)
                {
                    keysToDelete[keysToDeleteCount] = key;
                    keysToDeleteCount++;
                }

                if(keysToDeleteCount >= keyDeleteSize)
                    break;
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
