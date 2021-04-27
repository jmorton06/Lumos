#include "Precompiled.h"
#include "VKFramebuffer.h"
#include "VKDevice.h"
#include "VKTexture.h"
#include "VKInitialisers.h"
#include "VKTools.h"

namespace Lumos
{
    namespace Graphics
    {
        VKFramebuffer::VKFramebuffer(const FramebufferInfo& frameBufferInfo)
        {
            m_Width = frameBufferInfo.width;
            m_Height = frameBufferInfo.height;

            m_AttachmentCount = frameBufferInfo.attachmentCount;

            std::vector<VkImageView> attachments;

            for(uint32_t i = 0; i < m_AttachmentCount; i++)
            {
                switch(frameBufferInfo.attachmentTypes[i])
                {
                case TextureType::COLOUR:
                    attachments.push_back(static_cast<VKTexture2D*>(frameBufferInfo.attachments[i])->GetImageView());
                    break;
                case TextureType::DEPTH:
                    attachments.push_back(static_cast<VKTextureDepth*>(frameBufferInfo.attachments[i])->GetImageView());
                    break;
                case TextureType::DEPTHARRAY:
                    attachments.push_back(static_cast<VKTextureDepthArray*>(frameBufferInfo.attachments[i])->GetImageView(frameBufferInfo.layer));
                    break;
                case TextureType::OTHER:
                    attachments.push_back(static_cast<VKTexture2D*>(frameBufferInfo.attachments[i])->GetImageView());
                    break;
                case TextureType::CUBE:
                    UNIMPLEMENTED;
                    break;
                }
            }

            VkFramebufferCreateInfo framebufferCreateInfo = VKInitialisers::framebufferCreateInfo();

            framebufferCreateInfo.renderPass = static_cast<VKRenderpass*>(frameBufferInfo.renderPass)->GetHandle();
            framebufferCreateInfo.attachmentCount = m_AttachmentCount;
            framebufferCreateInfo.pAttachments = attachments.data();
            framebufferCreateInfo.width = m_Width;
            framebufferCreateInfo.height = m_Height;
            framebufferCreateInfo.layers = 1;
            framebufferCreateInfo.pNext = nullptr;
            framebufferCreateInfo.flags = 0;

            VK_CHECK_RESULT(vkCreateFramebuffer(VKDevice::Get().GetDevice(), &framebufferCreateInfo, VK_NULL_HANDLE, &m_Framebuffer));
        }

        VKFramebuffer::~VKFramebuffer()
        {
            vkDestroyFramebuffer(VKDevice::Get().GetDevice(), m_Framebuffer, VK_NULL_HANDLE);
        }

        void VKFramebuffer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        Framebuffer* VKFramebuffer::CreateFuncVulkan(const FramebufferInfo& info)
        {
            return new VKFramebuffer(info);
        }
    }
}
