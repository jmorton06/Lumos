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
            m_ClearValue      = NULL;
            m_ClearDepth      = false;
            m_SwapchainTarget = false;

            Init(renderPassDesc);
        }

        VKRenderPass::~VKRenderPass()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            delete[] m_ClearValue;

            DeletionQueue& deletionQueue = VKRenderer::GetCurrentDeletionQueue();
            VkRenderPass renderPass      = m_RenderPass;

            deletionQueue.PushFunction([renderPass]
                                       { vkDestroyRenderPass(VKDevice::Get().GetDevice(), renderPass, VK_NULL_HANDLE); });
        }

        VkAttachmentDescription GetAttachmentDescription(TextureType type, Texture* texture, bool clear = true)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            VkAttachmentDescription attachment = {};
            if(type == TextureType::COLOUR)
            {
                VKTexture2D* colourTexture = ((VKTexture2D*)texture);
                attachment.format          = colourTexture->GetVKFormat();
                attachment.initialLayout   = colourTexture->GetImageLayout();
                attachment.finalLayout     = attachment.initialLayout;
            }
            else if(type == TextureType::CUBE)
            {
                VKTextureCube* colourTexture = ((VKTextureCube*)texture);
                attachment.format            = colourTexture->GetVKFormat();
                attachment.initialLayout     = colourTexture->GetImageLayout();
                attachment.finalLayout       = attachment.initialLayout;
            }
            else if(type == TextureType::DEPTH)
            {
                attachment.format        = ((VKTextureDepth*)texture)->GetVKFormat();
                attachment.initialLayout = ((VKTextureDepth*)texture)->GetImageLayout();
                attachment.finalLayout   = attachment.initialLayout;
            }
            else if(type == TextureType::DEPTHARRAY)
            {
                attachment.format        = ((VKTextureDepthArray*)texture)->GetVKFormat();
                attachment.initialLayout = ((VKTextureDepthArray*)texture)->GetImageLayout();
                attachment.finalLayout   = attachment.initialLayout;
            }
            else
            {
                LUMOS_LOG_CRITICAL("[VULKAN] - Unsupported TextureType - {0}", static_cast<int>(type));
                return attachment;
            }

            if(clear)
            {
                attachment.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            else
            {
                attachment.loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            }

            attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
            attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.flags          = 0;

            return attachment;
        }

        bool VKRenderPass::Init(const RenderPassDesc& renderPassDesc)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            std::vector<VkAttachmentDescription> attachments;

            std::vector<VkAttachmentReference> colourAttachmentReferences;
            std::vector<VkAttachmentReference> depthAttachmentReferences;
            std::vector<VkSubpassDependency> dependencies;

            m_DepthOnly  = true;
            m_ClearDepth = false;
            m_DebugName  = renderPassDesc.DebugName;

            for(uint32_t i = 0; i < renderPassDesc.attachmentCount; i++)
            {
                attachments.push_back(GetAttachmentDescription(renderPassDesc.attachmentTypes[i], renderPassDesc.attachments[i], renderPassDesc.clear));

                if(renderPassDesc.attachmentTypes[i] == TextureType::COLOUR)
                {
                    VkImageLayout layout                      = ((VKTexture2D*)renderPassDesc.attachments[i])->GetImageLayout();
                    VkAttachmentReference colourAttachmentRef = {};
                    colourAttachmentRef.attachment            = uint32_t(i);
                    colourAttachmentRef.layout                = layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
                    colourAttachmentReferences.push_back(colourAttachmentRef);
                    m_DepthOnly = false;
                }
                else if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTH)
                {
                    VkAttachmentReference depthAttachmentRef = {};
                    depthAttachmentRef.attachment            = uint32_t(i);
                    depthAttachmentRef.layout                = ((VKTextureDepth*)renderPassDesc.attachments[i])->GetImageLayout();
                    depthAttachmentReferences.push_back(depthAttachmentRef);
                    m_ClearDepth = renderPassDesc.clear;
                }
                else if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTHARRAY)
                {
                    VkAttachmentReference depthAttachmentRef = {};
                    depthAttachmentRef.attachment            = uint32_t(i);
                    depthAttachmentRef.layout                = ((VKTextureDepthArray*)renderPassDesc.attachments[i])->GetImageLayout();
                    depthAttachmentReferences.push_back(depthAttachmentRef);
                    m_ClearDepth = renderPassDesc.clear;
                }
                else if(renderPassDesc.attachmentTypes[i] == TextureType::CUBE)
                {
                    VkImageLayout layout                      = ((VKTextureCube*)renderPassDesc.attachments[i])->GetImageLayout();
                    VkAttachmentReference colourAttachmentRef = {};
                    colourAttachmentRef.attachment            = uint32_t(i);
                    colourAttachmentRef.layout                = layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
                    colourAttachmentReferences.push_back(colourAttachmentRef);
                    m_DepthOnly = false;
                }
                else
                {
                    LUMOS_LOG_ERROR("Unsupported texture attachment");
                }

                if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTH || renderPassDesc.attachmentTypes[i] == TextureType::DEPTHARRAY)
                {
                    {
                        VkSubpassDependency& depedency = dependencies.emplace_back();
                        depedency.srcSubpass           = VK_SUBPASS_EXTERNAL;
                        depedency.dstSubpass           = 0;
                        depedency.srcStageMask         = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        depedency.dstStageMask         = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                        depedency.srcAccessMask        = VK_ACCESS_SHADER_READ_BIT;
                        depedency.dstAccessMask        = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        depedency.dependencyFlags      = VK_DEPENDENCY_BY_REGION_BIT;
                    }

                    {
                        VkSubpassDependency& depedency = dependencies.emplace_back();
                        depedency.srcSubpass           = 0;
                        depedency.dstSubpass           = VK_SUBPASS_EXTERNAL;
                        depedency.srcStageMask         = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                        depedency.dstStageMask         = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        depedency.srcAccessMask        = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        depedency.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
                        depedency.dependencyFlags      = VK_DEPENDENCY_BY_REGION_BIT;
                    }
                }
                else
                {
                    {
                        VkSubpassDependency& depedency = dependencies.emplace_back();
                        depedency.srcSubpass           = VK_SUBPASS_EXTERNAL;
                        depedency.dstSubpass           = 0;
                        depedency.srcStageMask         = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        depedency.srcAccessMask        = VK_ACCESS_SHADER_READ_BIT;
                        depedency.dstStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                        depedency.dstAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        depedency.dependencyFlags      = VK_DEPENDENCY_BY_REGION_BIT;
                    }
                    {
                        VkSubpassDependency& depedency = dependencies.emplace_back();
                        depedency.srcSubpass           = 0;
                        depedency.dstSubpass           = VK_SUBPASS_EXTERNAL;
                        depedency.srcStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                        depedency.srcAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        depedency.dstStageMask         = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        depedency.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
                        depedency.dependencyFlags      = VK_DEPENDENCY_BY_REGION_BIT;
                    }
                }
            }

            VkSubpassDescription subpass    = {};
            subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount    = static_cast<uint32_t>(colourAttachmentReferences.size());
            subpass.pColorAttachments       = colourAttachmentReferences.data();
            subpass.pDepthStencilAttachment = depthAttachmentReferences.data();

            m_ColourAttachmentCount = int(colourAttachmentReferences.size());

            VkRenderPassCreateInfo renderPassCreateInfo = VKInitialisers::RenderPassCreateInfo();
            renderPassCreateInfo.attachmentCount        = uint32_t(renderPassDesc.attachmentCount);
            renderPassCreateInfo.pAttachments           = attachments.data();
            renderPassCreateInfo.subpassCount           = 1;
            renderPassCreateInfo.pSubpasses             = &subpass;
            renderPassCreateInfo.dependencyCount        = 0;       // static_cast<uint32_t>(dependencies.size());
            renderPassCreateInfo.pDependencies          = nullptr; // dependencies.data();

            VK_CHECK_RESULT(vkCreateRenderPass(VKDevice::Get().GetDevice(), &renderPassCreateInfo, VK_NULL_HANDLE, &m_RenderPass));

            if(!renderPassDesc.DebugName.empty())
                VKUtilities::SetDebugUtilsObjectName(VKDevice::Get().GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, renderPassDesc.DebugName.c_str(), m_RenderPass);

            m_ClearValue      = new VkClearValue[renderPassDesc.attachmentCount];
            m_ClearCount      = renderPassDesc.attachmentCount;
            m_SwapchainTarget = renderPassDesc.swapchainTarget;

            return true;
        }

        VkSubpassContents SubPassContentsToVK(SubPassContents contents)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
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

        void VKRenderPass::BeginRenderPass(CommandBuffer* commandBuffer, float* clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(!m_DepthOnly)
            {
                for(int i = 0; i < m_ClearCount; i++)
                {
                    m_ClearValue[i].color.float32[0] = clearColour[0];
                    m_ClearValue[i].color.float32[1] = clearColour[1];
                    m_ClearValue[i].color.float32[2] = clearColour[2];
                    m_ClearValue[i].color.float32[3] = clearColour[3];
                }
            }

            if(m_ClearDepth)
            {
                m_ClearValue[m_ClearCount - 1].depthStencil = VkClearDepthStencilValue { 1.0f, 0 };
            }

            VkRenderPassBeginInfo rpBegin    = {};
            rpBegin.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpBegin.pNext                    = NULL;
            rpBegin.renderPass               = m_RenderPass;
            rpBegin.framebuffer              = static_cast<VKFramebuffer*>(frame)->GetFramebuffer();
            rpBegin.renderArea.offset.x      = 0;
            rpBegin.renderArea.offset.y      = 0;
            rpBegin.renderArea.extent.width  = width;
            rpBegin.renderArea.extent.height = height;
            rpBegin.clearValueCount          = uint32_t(m_ClearCount);
            rpBegin.pClearValues             = m_ClearValue;

            if(!m_DebugName.empty())
            {
                VkDebugUtilsLabelEXT debugLabel {};
                debugLabel.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
                debugLabel.pLabelName = m_DebugName.c_str();
                fpCmdBeginDebugUtilsLabelEXT(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), &debugLabel);
            }

            vkCmdBeginRenderPass(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), &rpBegin, SubPassContentsToVK(contents));
            commandBuffer->UpdateViewport(width, height, m_SwapchainTarget);
        }

        void VKRenderPass::EndRenderPass(CommandBuffer* commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            vkCmdEndRenderPass(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle());

            if(!m_DebugName.empty())
            {
                fpCmdEndDebugUtilsLabelEXT(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle());
            }
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
