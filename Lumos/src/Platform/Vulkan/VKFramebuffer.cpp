#include "LM.h"
#include "VKFramebuffer.h"
#include "VKDevice.h"
#include "VKTextureDepth.h"
#include "VKTextureDepthArray.h"


namespace Lumos
{
	namespace Graphics
	{
		VKFramebuffer::VKFramebuffer(FramebufferInfo frameBufferInfo)
		{
			m_Width  = frameBufferInfo.width;
			m_Height = frameBufferInfo.height;

			m_AttachmentCount = frameBufferInfo.attachmentCount;

			std::vector<vk::ImageView> attachments;

			for(u32 i = 0; i < m_AttachmentCount; i++)
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

			vk::FramebufferCreateInfo fbCI{};

			if(frameBufferInfo.renderPass)
				fbCI.renderPass = static_cast<VKRenderpass*>(frameBufferInfo.renderPass)->GetRenderpass();

			fbCI.attachmentCount = m_AttachmentCount;
			fbCI.pAttachments = attachments.data();
			fbCI.width = m_Width;
			fbCI.height = m_Height;
			fbCI.layers = 1;//frameBufferInfo.layers;

			m_Framebuffer = VKDevice::Instance()->GetDevice().createFramebuffer(fbCI);
		}

		VKFramebuffer::~VKFramebuffer()
		{
			VKDevice::Instance()->GetDevice().destroyFramebuffer(m_Framebuffer);
		}
	}
}
