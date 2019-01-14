#include "JM.h"
#include "VKFramebuffer.h"
#include "VKDevice.h"
#include "VKTextureDepth.h"
#include "VKTextureDepthArray.h"


namespace jm
{
	namespace graphics
	{
		VKFrameBuffer::VKFrameBuffer(FrameBufferInfo frameBufferInfo)
		{
			m_Width  = frameBufferInfo.width;
			m_Height = frameBufferInfo.height;

			m_AttachmentCount = frameBufferInfo.attachmentCount;

			std::vector<VkImageView> attachments;

			for(uint i = 0; i < m_AttachmentCount; i++)
			{
				switch (frameBufferInfo.attachmentTypes[i])
				{
				case TextureType::COLOUR 		: attachments.push_back(static_cast<VKTexture2D*>(frameBufferInfo.attachments[i])->GetImageView()); break;
				case TextureType::DEPTH  		: attachments.push_back(static_cast<VKTextureDepth*>(frameBufferInfo.attachments[i])->GetImageView()); break;
				case TextureType::DEPTHARRAY  	: attachments.push_back(static_cast<VKTextureDepthArray*>(frameBufferInfo.attachments[i])->GetImageView(frameBufferInfo.layer)); break;
				case TextureType::OTHER  		: attachments.push_back(static_cast<VKTexture2D*>(frameBufferInfo.attachments[i])->GetImageView()); break;
                case TextureType::CUBE          : UNIMPLEMENTED; break;
				}
			}

			VkFramebufferCreateInfo fbCI{};
			fbCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

			if(frameBufferInfo.renderPass)
				fbCI.renderPass = static_cast<VKRenderpass*>(frameBufferInfo.renderPass)->GetRenderpass();

			fbCI.attachmentCount = m_AttachmentCount;
			fbCI.pAttachments = attachments.data();
			fbCI.width = m_Width;
			fbCI.height = m_Height;
			fbCI.layers = 1;//frameBufferInfo.layers;

			vkCreateFramebuffer(VKDevice::Instance()->GetDevice(), &fbCI, VK_NULL_HANDLE, &m_Framebuffer);
		}

		VKFrameBuffer::~VKFrameBuffer()
		{
			vkDestroyFramebuffer(VKDevice::Instance()->GetDevice(), m_Framebuffer, VK_NULL_HANDLE);
		}
	}
}
