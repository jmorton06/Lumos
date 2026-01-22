#include "Precompiled.h"
#include "Framebuffer.h"
#include "Texture.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Core/Engine.h"

#include "Utilities/CombineHash.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTexture.h"
#endif

namespace Lumos
{
    namespace Graphics
    {
        Framebuffer* (*Framebuffer::CreateFunc)(const FramebufferDesc&) = nullptr;

        Framebuffer* Framebuffer::Create(const FramebufferDesc& framebufferDesc)
        {
            ASSERT(CreateFunc, "No Framebuffer Create Function");

            return CreateFunc(framebufferDesc);
        }

        struct FramebufferAsset
        {
            SharedPtr<Framebuffer> framebuffer;
            float timeSinceLastAccessed;
        };
        static std::unordered_map<uint64_t, FramebufferAsset> m_FramebufferCache;
        static const float m_CacheLifeTime = 0.0f;

        SharedPtr<Framebuffer> Framebuffer::Get(const FramebufferDesc& framebufferDesc)
        {
            LUMOS_PROFILE_FUNCTION();
            uint64_t hash = 0;
            HashCombine(hash, framebufferDesc.attachmentCount, framebufferDesc.width, framebufferDesc.height, framebufferDesc.layer, framebufferDesc.screenFBO, framebufferDesc.samples);

            for(uint32_t i = 0; i < framebufferDesc.attachmentCount; i++)
            {
                HashCombine(hash, framebufferDesc.attachmentTypes[i]);

                if(framebufferDesc.attachments[i])
                {
                    HashCombine(hash, framebufferDesc.attachments[i]->GetImageHande());
                    HashCombine(hash, framebufferDesc.attachments[i]->GetUUID());
                }
            }

            auto found = m_FramebufferCache.find(hash);
            if(found != m_FramebufferCache.end() && found->second.framebuffer)
            {
                found->second.timeSinceLastAccessed = (float)Engine::GetTimeStep().GetElapsedSeconds();
                return found->second.framebuffer;
            }

            auto framebuffer         = SharedPtr<Framebuffer>(Create(framebufferDesc));
            m_FramebufferCache[hash] = { framebuffer, (float)Engine::GetTimeStep().GetElapsedSeconds() };
            return framebuffer;
        }

        void Framebuffer::ClearCache()
        {
            LUMOS_PROFILE_FUNCTION();
            m_FramebufferCache.clear();
        }

        void Framebuffer::DeleteUnusedCache()
        {
            LUMOS_PROFILE_FUNCTION();
            static const size_t keyDeleteSize = 256;
            static std::size_t keysToDelete[keyDeleteSize];
            std::size_t keysToDeleteCount = 0;

            for(auto&& [key, value] : m_FramebufferCache)
            {
                if(!value.framebuffer)
                {
                    keysToDelete[keysToDeleteCount] = key;
                    keysToDeleteCount++;
                }
                else if(value.framebuffer.GetCounter() && value.framebuffer.GetCounter()->GetReferenceCount() == 1 && (Engine::GetTimeStep().GetElapsedSeconds() - value.timeSinceLastAccessed) > m_CacheLifeTime)
                {
                    keysToDelete[keysToDeleteCount] = key;
                    keysToDeleteCount++;
                }

                if(keysToDeleteCount >= keyDeleteSize)
                    break;
            }

            for(std::size_t i = 0; i < keysToDeleteCount; i++)
            {
                m_FramebufferCache[keysToDelete[i]].framebuffer = nullptr;
                m_FramebufferCache.erase(keysToDelete[i]);
            }
        }

        Framebuffer::~Framebuffer()
        {
        }
    }
}
