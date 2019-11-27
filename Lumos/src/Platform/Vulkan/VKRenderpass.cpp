#include "lmpch.h"
#include "VKRenderpass.h"
#include "VKCommandBuffer.h"
#include "VKFramebuffer.h"
#include "VKRenderer.h"


namespace Lumos
{
	namespace Graphics
	{

		VKRenderpass::VKRenderpass(): m_ClearCount(0), m_DepthOnly(false)
		{
			m_ClearValue = NULL;
			m_ClearDepth = false;
		}

		VKRenderpass::~VKRenderpass()
		{
			delete[] m_ClearValue;
			Unload();
		}

		VkAttachmentDescription GetAttachmentDescription(AttachmentInfo info, bool clear = true)
		{
			if (info.textureType == TextureType::COLOUR)
			{
				VkAttachmentDescription colorAttachment = {};
				colorAttachment.format = VKTools::TextureFormatToVK(info.format);
				colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

				return colorAttachment;
			}
			else if (info.textureType == TextureType::DEPTH)
			{
				VkAttachmentDescription depthAttachment = {};
				depthAttachment.format = VKTools::FindDepthFormat();
				depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				depthAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				return depthAttachment;
			}
			else if (info.textureType == TextureType::DEPTHARRAY)
			{
				VkAttachmentDescription depthAttachment = {};
				depthAttachment.format = VKTools::FindDepthFormat();
				depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				depthAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;;
				depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				return depthAttachment;
			}
			else
			{
				VkAttachmentDescription Attachment = {};
				Debug::Log::Critical("[VULKAN] - Unsupported TextureType - {0}", static_cast<int>(info.textureType));
				return Attachment;
			}
		}

		bool VKRenderpass::Init(const RenderpassInfo& renderpassCI)
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

			m_DepthOnly = true;
			m_ClearDepth = false;

			for(int i = 0; i < renderpassCI.attachmentCount;i++)
			{
				attachments.push_back(GetAttachmentDescription(renderpassCI.textureType[i], renderpassCI.clear));

				if(renderpassCI.textureType[i].textureType == TextureType::COLOUR)
				{
					VkAttachmentReference colourAttachmentRef = {};
					colourAttachmentRef.attachment = i;
					colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					colourAttachmentReferences.push_back(colourAttachmentRef);
					m_DepthOnly = false;
				}
				else if (renderpassCI.textureType[i].textureType == TextureType::DEPTH)
				{
					VkAttachmentReference depthAttachmentRef = {};
					depthAttachmentRef.attachment = i;
					depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthAttachmentReferences.push_back(depthAttachmentRef);
					m_ClearDepth = true;
				}
				else if (renderpassCI.textureType[i].textureType == TextureType::DEPTHARRAY)
				{
					VkAttachmentReference depthAttachmentRef = {};
					depthAttachmentRef.attachment = i;
					depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthAttachmentReferences.push_back(depthAttachmentRef);
					m_ClearDepth = true;
				}
			}

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = static_cast<u32>(colourAttachmentReferences.size());
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

			m_ClearValue = lmnew VkClearValue[renderpassCI.attachmentCount];
			m_ClearCount = renderpassCI.attachmentCount;
			return true;
		}

		void VKRenderpass::Unload() const
		{
			vkDestroyRenderPass(VKDevice::Instance()->GetDevice(), m_RenderPass, VK_NULL_HANDLE);
		}

		VkSubpassContents SubPassContentsToVK(SubPassContents contents)
		{
			switch(contents)
			{
			case INLINE: return VK_SUBPASS_CONTENTS_INLINE;
			case SECONDARY: return VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
			default: return VK_SUBPASS_CONTENTS_INLINE;
			}
		}

		void VKRenderpass::BeginRenderpass(CommandBuffer* commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame,
		                                   SubPassContents contents, uint32_t width, uint32_t height) const
		{
			if(!m_DepthOnly)
			{
				for (int i = 0; i < m_ClearCount; i++)
				{
					m_ClearValue[i].color.float32[0] = clearColour.x;
					m_ClearValue[i].color.float32[1] = clearColour.y;
					m_ClearValue[i].color.float32[2] = clearColour.z;
					m_ClearValue[i].color.float32[3] = clearColour.w;
				}
			}

			if (m_ClearDepth)
			{
                m_ClearValue[m_ClearCount - 1].depthStencil = VkClearDepthStencilValue{ 1.0f , 0 };
			}

			VkRenderPassBeginInfo rpBegin{};
			rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
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

			vkCmdBeginRenderPass(static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer(), &rpBegin, SubPassContentsToVK(contents));
		}

		void VKRenderpass::EndRenderpass(CommandBuffer* commandBuffer)
		{
			vkCmdEndRenderPass(reinterpret_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer());
		}
        
        void VKRenderpass::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }
        
		RenderPass* VKRenderpass::CreateFuncVulkan()
        {
            return lmnew VKRenderpass();
        }
	}
}
