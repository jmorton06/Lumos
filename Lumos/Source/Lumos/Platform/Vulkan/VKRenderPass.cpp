#include "Precompiled.h"
#include "VKRenderPass.h"
#include "VKCommandBuffer.h"
#include "VKFramebuffer.h"
#include "VKRenderer.h"
#include "VKInitialisers.h"
#include "VKUtilities.h"
#include "VKContext.h"
#include "Core/Application.h"
#include "Core/OS/Window.h"

namespace Lumos
{
    namespace Graphics
    {

        VKRenderPass::VKRenderPass(const RenderPassDesc& renderPassDesc)
            : m_ClearCount(0)
            , m_DepthOnly(false)
        {
            m_ClearValue = NULL;
            m_ClearDepth = false;

            Init(renderPassDesc);
        }

        VKRenderPass::~VKRenderPass()
        {
            LUMOS_PROFILE_FUNCTION();
            vkDeviceWaitIdle(VKDevice::GetHandle());
            delete[] m_ClearValue;

            VKContext::DeletionQueue& deletionQueue = VKRenderer::GetCurrentDeletionQueue();

            auto renderPass = m_RenderPass;

            deletionQueue.PushFunction([renderPass]
                { vkDestroyRenderPass(VKDevice::Get().GetDevice(), renderPass, VK_NULL_HANDLE); });
        }

        VkAttachmentDescription GetAttachmentDescription(TextureType type, Texture* texture, bool clear = true)
        {
            LUMOS_PROFILE_FUNCTION();
            VkAttachmentDescription attachment = {};
            if(type == TextureType::COLOUR)
            {
                VKTexture2D* colourTexture = ((VKTexture2D*)texture);
                attachment.format = colourTexture->GetVKFormat();
                attachment.initialLayout = colourTexture->GetImageLayout();
                attachment.finalLayout = attachment.initialLayout;
            }
            else if(type == TextureType::DEPTH)
            {
                attachment.format = VKUtilities::FindDepthFormat();
                attachment.initialLayout = ((VKTextureDepth*)texture)->GetImageLayout();
                attachment.finalLayout = attachment.initialLayout;
            }
            else if(type == TextureType::DEPTHARRAY)
            {
                attachment.format = VKUtilities::FindDepthFormat();
                attachment.initialLayout = ((VKTextureDepthArray*)texture)->GetImageLayout();
                attachment.finalLayout = attachment.initialLayout;
            }
            else
            {
                LUMOS_LOG_CRITICAL("[VULKAN] - Unsupported TextureType - {0}", static_cast<int>(type));
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
            }

            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.flags = 0;

            return attachment;
        }

        bool VKRenderPass::Init(const RenderPassDesc& renderPassDesc)
        {
            LUMOS_PROFILE_FUNCTION();
            std::vector<VkAttachmentDescription> attachments;

            std::vector<VkAttachmentReference> colourAttachmentReferences;
            std::vector<VkAttachmentReference> depthAttachmentReferences;

            m_DepthOnly = true;
            m_ClearDepth = false;

            for(uint32_t i = 0; i < renderPassDesc.attachmentCount; i++)
            {
                attachments.push_back(GetAttachmentDescription(renderPassDesc.attachmentTypes[i], renderPassDesc.attachments[i], renderPassDesc.clear));

                if(renderPassDesc.attachmentTypes[i] == TextureType::COLOUR)
                {
                    VkImageLayout layout = ((VKTexture2D*)renderPassDesc.attachments[i])->GetImageLayout();
                    VkAttachmentReference colourAttachmentRef = {};
                    colourAttachmentRef.attachment = uint32_t(i);
                    colourAttachmentRef.layout = layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
                    colourAttachmentReferences.push_back(colourAttachmentRef);
                    m_DepthOnly = false;
                }
                else if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTH)
                {
                    VkAttachmentReference depthAttachmentRef = {};
                    depthAttachmentRef.attachment = uint32_t(i);
                    depthAttachmentRef.layout = ((VKTextureDepth*)renderPassDesc.attachments[i])->GetImageLayout();
                    depthAttachmentReferences.push_back(depthAttachmentRef);
                    m_ClearDepth = renderPassDesc.clear;
                }
                else if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTHARRAY)
                {
                    VkAttachmentReference depthAttachmentRef = {};
                    depthAttachmentRef.attachment = uint32_t(i);
                    depthAttachmentRef.layout = ((VKTextureDepthArray*)renderPassDesc.attachments[i])->GetImageLayout();
                    depthAttachmentReferences.push_back(depthAttachmentRef);
                    m_ClearDepth = renderPassDesc.clear;
                }
                else
                {
                    LUMOS_LOG_ERROR("Unsupported texture attachment");
                }
            }

            VkSubpassDescription subpass {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = static_cast<uint32_t>(colourAttachmentReferences.size());
            subpass.pColorAttachments = colourAttachmentReferences.data();
            subpass.pDepthStencilAttachment = depthAttachmentReferences.data();

            m_ColourAttachmentCount = int(colourAttachmentReferences.size());

            VkRenderPassCreateInfo renderPassCreateInfo = VKInitialisers::renderPassCreateInfo();
            renderPassCreateInfo.attachmentCount = uint32_t(renderPassDesc.attachmentCount);
            renderPassCreateInfo.pAttachments = attachments.data();
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = &subpass;
            renderPassCreateInfo.dependencyCount = 0;
            renderPassCreateInfo.pDependencies = nullptr;

            VK_CHECK_RESULT(vkCreateRenderPass(VKDevice::Get().GetDevice(), &renderPassCreateInfo, VK_NULL_HANDLE, &m_RenderPass));

            m_ClearValue = new VkClearValue[renderPassDesc.attachmentCount];
            m_ClearCount = renderPassDesc.attachmentCount;
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

        void VKRenderPass::BeginRenderpass(CommandBuffer* commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const
        {
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
            commandBuffer->UpdateViewport(width, height);
        }

        void VKRenderPass::EndRenderpass(CommandBuffer* commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            vkCmdEndRenderPass(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle());
        }

        void VKRenderPass::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        RenderPass* VKRenderPass::CreateFuncVulkan(const RenderPassDesc& renderPassDesc)
        {
            return new VKRenderPass(renderPassDesc);
        }
    }
}
