#include "Precompiled.h"
#include "Framebuffer.h"
#include "Texture.h"
#include "Graphics/API/GraphicsContext.h"

#include "Utilities/CombineHash.h"

namespace Lumos
{
	namespace Graphics
	{
        Framebuffer*(*Framebuffer::CreateFunc)(const FramebufferInfo&) = nullptr;

		Framebuffer* Framebuffer::Create(const FramebufferInfo& framebufferInfo)
		{
            LUMOS_ASSERT(CreateFunc, "No Framebuffer Create Function");
            
            return CreateFunc(framebufferInfo);
		}
    
        static std::unordered_map<std::size_t, Ref<Framebuffer>> m_FramebufferCache;
    
        Ref<Framebuffer> Framebuffer::Get(const FramebufferInfo& framebufferInfo)
        {
            size_t hash = 0;
            HashCombine(hash, framebufferInfo.attachmentCount, framebufferInfo.width, framebufferInfo.height, framebufferInfo.layer, framebufferInfo.renderPass, framebufferInfo.screenFBO);
            
            for(u32 i = 0; i < framebufferInfo.attachmentCount; i++)
            {
                HashCombine(hash, framebufferInfo.attachmentTypes[i], framebufferInfo.attachments[i]->GetHandle());
            }
            
            auto found = m_FramebufferCache.find(hash);
            if (found != m_FramebufferCache.end())
            {
                //Disable until fix resize issue. 
               // return found->second;
            }
            
            auto framebuffer = Ref<Framebuffer>(Create(framebufferInfo));
            m_FramebufferCache[hash] = framebuffer;
            return framebuffer;
        }
    
        void Framebuffer::ClearCache()
        {
            m_FramebufferCache.clear();
        }

		Framebuffer::~Framebuffer()
		{
		}
	}
}

