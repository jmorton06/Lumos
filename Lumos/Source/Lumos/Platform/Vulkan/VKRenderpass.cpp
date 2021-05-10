#include "Precompiled.h"
#include "VKRenderpass.h"
#include "VKCommandBuffer.h"
#include "VKFramebuffer.h"
#include "VKRenderer.h"
#include "VKInitialisers.h"
#include "VKTools.h"
#include "VKContext.h"

namespace Lumos
{
    namespace Graphics
    {

        VKRenderpass::VKRenderpass(const RenderPassInfo& renderPassCI)
            : m_ClearCount(0)
            , m_DepthOnly(false)
        {
            m_ClearValue = NULL;
            m_ClearDepth = false;

            Init(renderPassCI);
        }

        VKRenderpass::~VKRenderpass()
        {
            delete[] m_ClearValue;
            LUMOS_PROFILE_FUNCTION();
            vkDestroyRenderPass(VKDevice::Get().GetDevice(), m_RenderPass, VK_NULL_HANDLE);
        }

        VkAttachmentDescription GetAttachmentDescription(AttachmentInfo info, bool clear = true)
        {
            LUMOS_PROFILE_FUNCTION();
            VkAttachmentDescription attachment = {};
            if(info.textureType == TextureType::COLOUR)
            {
                attachment.format = info.format == TextureFormat::SCREEN ? VKContext::Get()->GetSwapchain()->GetScreenFormat() : VKTools::TextureFormatToVK(info.format, false);
                attachment.finalLayout = info.format == TextureFormat::SCREEN ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            else if(info.textureType == TextureType::DEPTH)
            {
                attachment.format = VKTools::FindDepthFormat();
                attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else if(info.textureType == TextureType::DEPTHARRAY)
            {
                attachment.format = VKTools::FindDepthFormat();
                attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else
            {
                LUMOS_LOG_CRITICAL("[VULKAN] - Unsupported TextureType - {0}", static_cast<int>(info.textureType));
                return attachment;
            }

            if(clear)
            {
                attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            else
            {
                attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.flags = 0;

            return attachment;
        }

        bool VKRenderpass::Init(const RenderPassInfo& renderpassCI)
        {
            LUMOS_PROFILE_FUNCTION();
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

            for(int i = 0; i < renderpassCI.attachmentCount; i++)
            {
                attachments.push_back(GetAttachmentDescription(renderpassCI.textureType[i], renderpassCI.clear));

                if(renderpassCI.textureType[i].textureType == TextureType::COLOUR)
                {
                    VkAttachmentReference colourAttachmentRef = {};
                    colourAttachmentRef.attachment = uint32_t(i);
                    colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    colourAttachmentReferences.push_back(colourAttachmentRef);
                    m_DepthOnly = false;
                }
                else if(renderpassCI.textureType[i].textureType == TextureType::DEPTH)
                {
                    VkAttachmentReference depthAttachmentRef = {};
                    depthAttachmentRef.attachment = uint32_t(i);
                    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    depthAttachmentReferences.push_back(depthAttachmentRef);
                    m_ClearDepth = renderpassCI.clear;
                }
                else if(renderpassCI.textureType[i].textureType == TextureType::DEPTHARRAY)
                {
                    VkAttachmentReference depthAttachmentRef = {};
                    depthAttachmentRef.attachment = uint32_t(i);
                    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    depthAttachmentReferences.push_back(depthAttachmentRef);
                    m_ClearDepth = renderpassCI.clear;
                }
            }

            VkSubpassDescription subpass {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = static_cast<uint32_t>(colourAttachmentReferences.size());
            subpass.pColorAttachments = colourAttachmentReferences.data();
            subpass.pDepthStencilAttachment = depthAttachmentReferences.data();

            m_ColourAttachmentCount = int(colourAttachmentReferences.size());

            VkRenderPassCreateInfo vkRenderpassCI = VKInitialisers::renderPassCreateInfo();
            vkRenderpassCI.attachmentCount = uint32_t(renderpassCI.attachmentCount);
            vkRenderpassCI.pAttachments = attachments.data();
            vkRenderpassCI.subpassCount = 1;
            vkRenderpassCI.pSubpasses = &subpass;
            vkRenderpassCI.dependencyCount = 0; //1;
            vkRenderpassCI.pDependencies = nullptr; //&dependency;

            VK_CHECK_RESULT(vkCreateRenderPass(VKDevice::Get().GetDevice(), &vkRenderpassCI, VK_NULL_HANDLE, &m_RenderPass));

            m_ClearValue = new VkClearValue[renderpassCI.attachmentCount];
            m_ClearCount = renderpassCI.attachmentCount;
            return true;
        }

        VkSubpassContents SubPassContentsToVK(SubPassContents contents)
        {
            LUMOS_PROFILE_FUNCTION();
            switch(contents)
            {
            case INLINE:
                return VK_SUBPASS_CONTENTS_INLINE;
            case SECONDARY:
                return VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
            default:
                return VK_SUBPASS_CONTENTS_INLINE;
            }
        }

        static bool inRenderPass = false;
        static const VKRenderpass* currentpsss = nullptr;

        void VKRenderpass::BeginRenderpass(CommandBuffer* commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const
        {
            commandBuffer->UpdateViewport(width, height);

            LUMOS_PROFILE_FUNCTION();
            if(!m_DepthOnly)
            {
                for(int i = 0; i < m_ClearCount; i++)
                {
                    m_ClearValue[i].color.float32[0] = clearColour.x;
                    m_ClearValue[i].color.float32[1] = clearColour.y;
                    m_ClearValue[i].color.float32[2] = clearColour.z;
                    m_ClearValue[i].color.float32[3] = clearColour.w;
                }
            }

            if(m_ClearDepth)
            {
                m_ClearValue[m_ClearCount - 1].depthStencil = VkClearDepthStencilValue { 1.0f, 0 };
            }

            VkRenderPassBeginInfo rpBegin {};
            rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpBegin.pNext = NULL;
            rpBegin.renderPass = m_RenderPass;
            rpBegin.framebuffer = static_cast<VKFramebuffer*>(frame)->GetFramebuffer();
            rpBegin.renderArea.offset.x = 0;
            rpBegin.renderArea.offset.y = 0;
            rpBegin.renderArea.extent.width = width;
            rpBegin.renderArea.extent.height = height;
            rpBegin.clearValueCount = uint32_t(m_ClearCount);
            rpBegin.pClearValues = m_ClearValue;

            vkCmdBeginRenderPass(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), &rpBegin, SubPassContentsToVK(contents));
        }

        void VKRenderpass::EndRenderpass(CommandBuffer* commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            vkCmdEndRenderPass(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle());
        }

        void VKRenderpass::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        RenderPass* VKRenderpass::CreateFuncVulkan(const RenderPassInfo& renderPassCI)
        {
            return new VKRenderpass(renderPassCI);
        }
    }
}
