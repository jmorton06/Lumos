#include "LM.h"
#include "VKRenderpass.h"
#include "VKCommandBuffer.h"
#include "VKFramebuffer.h"
#include "VKRenderer.h"


namespace Lumos
{
	namespace graphics
	{

		VKRenderpass::VKRenderpass()
		{
			m_ClearValue = NULL;
			m_ClearDepth = false;
		}

		VKRenderpass::~VKRenderpass()
		{
			delete[] m_ClearValue;
			Unload();
		}

		vk::AttachmentDescription GetAttachmentDescription(TextureType type, bool clear = true)
		{
			if (type == TextureType::COLOUR)
			{
				vk::AttachmentDescription colorAttachment = {};
				colorAttachment.format = VKDevice::Instance()->GetFormat();
				colorAttachment.samples = vk::SampleCountFlagBits::e1;
				colorAttachment.loadOp = clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
				colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
				colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
				colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
				colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
				colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
				return colorAttachment;
			}
			else if (type == TextureType::DEPTH)
			{
				vk::AttachmentDescription depthAttachment = {};
				depthAttachment.format = VKTools::FindDepthFormat();
				depthAttachment.samples = vk::SampleCountFlagBits::e1;
				depthAttachment.loadOp = clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
				depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
				depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
				depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
				depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
				depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				return depthAttachment;
			}
				else if (type == TextureType::DEPTHARRAY)
			{
				vk::AttachmentDescription depthAttachment = {};
				depthAttachment.format = VKTools::FindDepthFormat();
				depthAttachment.samples = vk::SampleCountFlagBits::e1;
				depthAttachment.loadOp = clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
				depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
				depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
				depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
				depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
				depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
				return depthAttachment;
			}
			else
			{
				vk::AttachmentDescription Attachment = {};
				LUMOS_CORE_ERROR("[VULKAN] - Unsupported TextureType - {0}", static_cast<int>(type));
				return Attachment;
			}
		}

		bool VKRenderpass::Init(const api::RenderpassInfo& renderpassCI)
		{
			vk::SubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
            //dependency.srcAccessMask = 0;
			dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

			std::vector<vk::AttachmentDescription> attachments;

			std::vector<vk::AttachmentReference> colourAttachmentReferences;
			std::vector<vk::AttachmentReference> depthAttachmentReferences;

			for(int i = 0; i < renderpassCI.attachmentCount;i++)
			{
				attachments.push_back(GetAttachmentDescription(renderpassCI.textureType[i], renderpassCI.clear));

				if(renderpassCI.textureType[i] == TextureType::COLOUR)
				{
					vk::AttachmentReference colourAttachmentRef = {};
					colourAttachmentRef.attachment = i;
					colourAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
					colourAttachmentReferences.push_back(colourAttachmentRef);
				}
				else if (renderpassCI.textureType[i] == TextureType::DEPTH)
				{
					vk::AttachmentReference depthAttachmentRef = {};
					depthAttachmentRef.attachment = i;
					depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					depthAttachmentReferences.push_back(depthAttachmentRef);
					m_ClearDepth = true;
				}
				else if (renderpassCI.textureType[i] == TextureType::DEPTHARRAY)
				{
					vk::AttachmentReference depthAttachmentRef = {};
					depthAttachmentRef.attachment = i;
					depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					depthAttachmentReferences.push_back(depthAttachmentRef);
					m_ClearDepth = true;
				}
			}

			vk::SubpassDescription subpass{};
			subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
			subpass.colorAttachmentCount = static_cast<uint>(colourAttachmentReferences.size());
			subpass.pColorAttachments = colourAttachmentReferences.data();
			subpass.pDepthStencilAttachment = depthAttachmentReferences.data();

			vk::RenderPassCreateInfo vkRenderpassCI{};
			vkRenderpassCI.attachmentCount = renderpassCI.attachmentCount;
			vkRenderpassCI.pAttachments = attachments.data();
			vkRenderpassCI.subpassCount = 1;
			vkRenderpassCI.pSubpasses = &subpass;
			vkRenderpassCI.dependencyCount = 1;
			vkRenderpassCI.pDependencies = &dependency;

			m_RenderPass = VKDevice::Instance()->GetDevice().createRenderPass(vkRenderpassCI);
			if (!m_RenderPass)
				return false;

			m_ClearValue = new vk::ClearValue[renderpassCI.attachmentCount];
			m_ClearCount = renderpassCI.attachmentCount;
			m_DepthOnly = renderpassCI.depthOnly;
			return true;
		}

		void VKRenderpass::Unload() const
		{
			VKDevice::Instance()->GetDevice().destroyRenderPass(m_RenderPass);
		}

		vk::SubpassContents SubPassContentsToVK(api::SubPassContents contents)
		{
			switch(contents)
			{
			case api::INLINE: return vk::SubpassContents::eInline;
			case api::SECONDARY : return vk::SubpassContents::eSecondaryCommandBuffers;
			default: return vk::SubpassContents::eInline;
			}
		}

		void VKRenderpass::BeginRenderpass(api::CommandBuffer* commandBuffer, const maths::Vector4& clearColour, Framebuffer* frame,
		                                   api::SubPassContents contents, uint32_t width, uint32_t height) const
		{
			if(!m_DepthOnly)
			{
				for (int i = 0; i < m_ClearCount; i++)
				{
					m_ClearValue[i].color.float32[0] = clearColour.GetX();
					m_ClearValue[i].color.float32[1] = clearColour.GetY();
					m_ClearValue[i].color.float32[2] = clearColour.GetZ();
					m_ClearValue[i].color.float32[3] = clearColour.GetW();
				}
			}

			if (m_ClearDepth)
			{
                m_ClearValue[m_ClearCount - 1].depthStencil = vk::ClearDepthStencilValue{ 1.0f , 0 };
			}

			vk::RenderPassBeginInfo rpBegin{};
			rpBegin.pNext = NULL;
			rpBegin.renderPass = m_RenderPass;
			if(frame)
				rpBegin.framebuffer = static_cast<VKFramebuffer*>(frame)->GetFramebuffer();
			rpBegin.renderArea.offset.x = 0;
			rpBegin.renderArea.offset.y = 0;
			rpBegin.renderArea.extent.width = width;
			rpBegin.renderArea.extent.height = height;
			rpBegin.clearValueCount = m_ClearCount;
			rpBegin.pClearValues = m_ClearValue;

			static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer().beginRenderPass(rpBegin, SubPassContentsToVK(contents));
		}

		void VKRenderpass::EndRenderpass(api::CommandBuffer* commandBuffer)
		{
			reinterpret_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer().endRenderPass();
		}
	}
}
