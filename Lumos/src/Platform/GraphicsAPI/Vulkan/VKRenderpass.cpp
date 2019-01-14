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
			m_RenderPass = VK_NULL_HANDLE;
			m_ClearValue = NULL;
		}

		VKRenderpass::~VKRenderpass()
		{
			delete[] m_ClearValue;
			Unload();
			m_RenderPass = VK_NULL_HANDLE;
		}

		VkAttachmentDescription GetAttachmentDescription(TextureType type)
		{
			if (type == TextureType::COLOUR)
			{
				VkAttachmentDescription colorAttachment = {};
				colorAttachment.format = VKDevice::Instance()->GetFormat();
				colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				return colorAttachment;
			}
			else if (type == TextureType::DEPTH)
			{
				VkAttachmentDescription depthAttachment = {};
				depthAttachment.format = VKTools::findDepthFormat();
				depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				return depthAttachment;
			}
				else if (type == TextureType::DEPTHARRAY)
			{
				VkAttachmentDescription depthAttachment = {};
				depthAttachment.format = VKTools::findDepthFormat();
				depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				return depthAttachment;
			}
			else
			{
				VkAttachmentDescription Attachment = {};
				LUMOS_CORE_ERROR("[VULKAN] - Unsupported TextureType - {0}", static_cast<int>(type));
				return Attachment;
			}
		}

		bool VKRenderpass::Init(const api::RenderpassInfo& renderpassCI)
		{
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			std::vector<VkAttachmentDescription> attachments;

			std::vector<VkAttachmentReference> colourAttachmentReferences;
			std::vector<VkAttachmentReference> depthAttachmentReferences;

			for(int i = 0; i < renderpassCI.attachmentCount;i++)
			{
				attachments.push_back(GetAttachmentDescription(renderpassCI.textureType[i]));

				if(renderpassCI.textureType[i] == TextureType::COLOUR)
				{
					VkAttachmentReference colourAttachmentRef = {};
					colourAttachmentRef.attachment = i;
					colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					colourAttachmentReferences.push_back(colourAttachmentRef);
				}
				else if (renderpassCI.textureType[i] == TextureType::DEPTH)
				{
					VkAttachmentReference depthAttachmentRef = {};
					depthAttachmentRef.attachment = i;
					depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthAttachmentReferences.push_back(depthAttachmentRef);
				}
				else if (renderpassCI.textureType[i] == TextureType::DEPTHARRAY)
				{
					VkAttachmentReference depthAttachmentRef = {};
					depthAttachmentRef.attachment = i;
					depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthAttachmentReferences.push_back(depthAttachmentRef);
				}
			}

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = static_cast<uint>(colourAttachmentReferences.size());
			subpass.pColorAttachments = colourAttachmentReferences.data();
			subpass.pDepthStencilAttachment = depthAttachmentReferences.data();

			VkRenderPassCreateInfo vkRenderpassCI{};
			vkRenderpassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			vkRenderpassCI.attachmentCount = renderpassCI.attachmentCount;
			vkRenderpassCI.pAttachments = attachments.data();
			vkRenderpassCI.subpassCount = 1;
			vkRenderpassCI.pSubpasses = &subpass;
			vkRenderpassCI.dependencyCount = 1;
			vkRenderpassCI.pDependencies = &dependency;

			VkResult result = vkCreateRenderPass(VKDevice::Instance()->GetDevice(), &vkRenderpassCI, VK_NULL_HANDLE, &m_RenderPass);
			if (result != VK_SUCCESS)
				return false;

			m_ClearValue = new VkClearValue[renderpassCI.attachmentCount];
			m_ClearCount = renderpassCI.attachmentCount;
			m_DepthOnly = renderpassCI.depthOnly;
			return true;
		}

		void VKRenderpass::Unload() const
		{
			vkDestroyRenderPass(VKDevice::Instance()->GetDevice(), m_RenderPass, VK_NULL_HANDLE);
		}

		VkSubpassContents SubPassContentsToVK(api::SubPassContents contents)
		{
			switch(contents)
			{
			case api::INLINE	: return VK_SUBPASS_CONTENTS_INLINE;
			case api::SECONDARY : return VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
			default: return VK_SUBPASS_CONTENTS_INLINE;
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


			m_ClearValue[m_ClearCount - 1].depthStencil = { 1.0f , 0 };

			VkRenderPassBeginInfo rpBegin{};
			rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rpBegin.pNext = NULL;
			rpBegin.renderPass = m_RenderPass;
			if(frame)
				rpBegin.framebuffer = static_cast<VKFrameBuffer*>(frame)->GetFrameBuffer();
			rpBegin.renderArea.offset.x = 0;
			rpBegin.renderArea.offset.y = 0;
			rpBegin.renderArea.extent.width = width;
			rpBegin.renderArea.extent.height = height;
			rpBegin.clearValueCount = m_ClearCount;
			rpBegin.pClearValues = m_ClearValue;


			vkCmdBeginRenderPass(static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer(), &rpBegin, SubPassContentsToVK(contents));
		}

		void VKRenderpass::EndRenderpass(api::CommandBuffer* commandBuffer)
		{
			vkCmdEndRenderPass(reinterpret_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer());
		}
	}
}
