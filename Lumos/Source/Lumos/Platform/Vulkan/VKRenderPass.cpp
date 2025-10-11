#include "Precompiled.h"
#include "VKRenderPass.h"
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKFramebuffer.h"
#include "VKRenderer.h"
#include "VKInitialisers.h"
#include "VKUtilities.h"
#include "VKTexture.h"
#include "VKContext.h"
#include "Core/Application.h"
#include "Core/OS/Window.h"
#include "Core/Engine.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    namespace Graphics
    {

        static uint32_t s_ActiveCount = 0;
        VKRenderPass::VKRenderPass(const RenderPassDesc& renderPassDesc)
            : m_ClearCount(0)
            , m_DepthOnly(false)
            , m_DepthAttachmentIndex(0)
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

        VkSampleCountFlagBits ConvertSampleCount(u8 samples)
        {
            switch(samples)
            {
            case 1:
                return VK_SAMPLE_COUNT_1_BIT;
            case 2:
                return VK_SAMPLE_COUNT_2_BIT;
            case 4:
                return VK_SAMPLE_COUNT_4_BIT;
            case 8:
                return VK_SAMPLE_COUNT_8_BIT;
            case 16:
                return VK_SAMPLE_COUNT_16_BIT;
            case 32:
                return VK_SAMPLE_COUNT_32_BIT;
            case 64:
                return VK_SAMPLE_COUNT_64_BIT;
            default:
                return VK_SAMPLE_COUNT_1_BIT;
            }
        }

        VkAttachmentDescription GetAttachmentDescription(TextureType type, Texture* texture, uint8_t samples = 1, bool clear = true, bool swapchainTarget = false)
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
                LFATAL("[VULKAN] - Unsupported TextureType - %i", static_cast<int>(type));
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

            attachment.samples = ConvertSampleCount(samples);

            attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.flags          = 0;

            if (type == TextureType::COLOUR && swapchainTarget)
           {
               attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
           }

            return attachment;
        }

        bool VKRenderPass::Init(const RenderPassDesc& renderPassDesc)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            TDArray<VkAttachmentDescription> attachments;

            TDArray<VkAttachmentReference> colourAttachmentReferences;
            TDArray<VkAttachmentReference> depthAttachmentReferences;
            TDArray<VkSubpassDependency> dependencies;

            m_DepthOnly  = true;
            m_ClearDepth = false;
            m_DebugName  = renderPassDesc.DebugName;

            for(uint32_t i = 0; i < renderPassDesc.attachmentCount; i++)
            {
                attachments.PushBack(GetAttachmentDescription(renderPassDesc.attachmentTypes[i], renderPassDesc.attachments[i], renderPassDesc.samples, renderPassDesc.clear, renderPassDesc.swapchainTarget));

                if(renderPassDesc.attachmentTypes[i] == TextureType::COLOUR)
                {
                    VkImageLayout layout                      = ((VKTexture2D*)renderPassDesc.attachments[i])->GetImageLayout();
                    VkAttachmentReference colourAttachmentRef = {};
                    colourAttachmentRef.attachment            = uint32_t(i);
                    colourAttachmentRef.layout                = layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
                    colourAttachmentReferences.PushBack(colourAttachmentRef);

                    if(attachments.Back().finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                    {
                        ((VKTexture2D*)renderPassDesc.attachments[i])->SetImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
                    }
                    m_DepthOnly = false;
                }
                else if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTH)
                {
                    VkAttachmentReference depthAttachmentRef = {};
                    depthAttachmentRef.attachment            = uint32_t(i);
                    depthAttachmentRef.layout                = ((VKTextureDepth*)renderPassDesc.attachments[i])->GetImageLayout();
                    depthAttachmentReferences.PushBack(depthAttachmentRef);
                    m_ClearDepth = renderPassDesc.clear;
                }
                else if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTHARRAY)
                {
                    VkAttachmentReference depthAttachmentRef = {};
                    depthAttachmentRef.attachment            = uint32_t(i);
                    depthAttachmentRef.layout                = ((VKTextureDepthArray*)renderPassDesc.attachments[i])->GetImageLayout();
                    depthAttachmentReferences.PushBack(depthAttachmentRef);
                    m_ClearDepth = renderPassDesc.clear;
                }
                else if(renderPassDesc.attachmentTypes[i] == TextureType::CUBE)
                {
                    VkImageLayout layout                      = ((VKTextureCube*)renderPassDesc.attachments[i])->GetImageLayout();
                    VkAttachmentReference colourAttachmentRef = {};
                    colourAttachmentRef.attachment            = uint32_t(i);
                    colourAttachmentRef.layout                = layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
                    colourAttachmentReferences.PushBack(colourAttachmentRef);
                    m_DepthOnly = false;
                }
                else
                {
                    LERROR("Unsupported texture attachment");
                }

                if(renderPassDesc.attachmentTypes[i] == TextureType::DEPTH || renderPassDesc.attachmentTypes[i] == TextureType::DEPTHARRAY)
                {
                    m_DepthAttachmentIndex = i;
                    {
                        VkSubpassDependency& depedency = dependencies.EmplaceBack();
                        depedency.srcSubpass           = VK_SUBPASS_EXTERNAL;
                        depedency.dstSubpass           = 0;
                        depedency.srcStageMask         = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        depedency.dstStageMask         = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                        depedency.srcAccessMask        = VK_ACCESS_SHADER_READ_BIT;
                        depedency.dstAccessMask        = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        depedency.dependencyFlags      = VK_DEPENDENCY_BY_REGION_BIT;
                    }

                    {
                        VkSubpassDependency& depedency = dependencies.EmplaceBack();
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
                        VkSubpassDependency& dependency = dependencies.EmplaceBack();
                        dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
                        dependency.dstSubpass      = 0;
                        dependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                        dependency.srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                        dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                    }

                    {
                        VkSubpassDependency& depedency = dependencies.EmplaceBack();
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

            uint32_t attachmentCount = renderPassDesc.attachmentCount;

            bool resolveTexture                               = false;
            VkAttachmentReference colourAttachmentResolvedRef = {};

            if(renderPassDesc.resolveTexture != nullptr && renderPassDesc.samples > 1)
            {
                resolveTexture                         = true;
                VkImageLayout layout                   = ((VKTexture2D*)renderPassDesc.resolveTexture)->GetImageLayout();
                colourAttachmentResolvedRef.attachment = uint32_t(renderPassDesc.attachmentCount);
                colourAttachmentResolvedRef.layout     = layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : layout;
                attachmentCount++;

                attachments.PushBack(GetAttachmentDescription(renderPassDesc.attachmentTypes[0], renderPassDesc.resolveTexture, 1, renderPassDesc.clear, renderPassDesc.swapchainTarget));
            }

            VkSubpassDescription subpass    = {};
            subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount    = static_cast<uint32_t>(colourAttachmentReferences.Size());
            subpass.pColorAttachments       = colourAttachmentReferences.Data();
            subpass.pDepthStencilAttachment = depthAttachmentReferences.Data();
            subpass.pResolveAttachments     = resolveTexture ? &colourAttachmentResolvedRef : nullptr;

            m_ColourAttachmentCount = int(colourAttachmentReferences.Size());

            VkRenderPassCreateInfo renderPassCreateInfo = VKInitialisers::RenderPassCreateInfo();
            renderPassCreateInfo.attachmentCount        = uint32_t(attachmentCount);
            renderPassCreateInfo.pAttachments           = attachments.Data();
            renderPassCreateInfo.subpassCount           = 1;
            renderPassCreateInfo.pSubpasses             = &subpass;
            renderPassCreateInfo.dependencyCount        = static_cast<uint32_t>(dependencies.Size());
            renderPassCreateInfo.pDependencies          = dependencies.Data();

            VK_CHECK_RESULT(vkCreateRenderPass(VKDevice::Get().GetDevice(), &renderPassCreateInfo, VK_NULL_HANDLE, &m_RenderPass));

            if(!renderPassDesc.DebugName.empty())
                VKUtilities::SetDebugUtilsObjectName(VKDevice::Get().GetDevice(), VK_OBJECT_TYPE_RENDER_PASS, renderPassDesc.DebugName.c_str(), m_RenderPass);

            m_ClearValue      = new VkClearValue[attachmentCount];
            m_ClearCount      = attachmentCount;
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
                m_ClearValue[m_DepthAttachmentIndex].depthStencil = VkClearDepthStencilValue { 1.0f, 0 };
            }

            u32 RenderPassWidth  = Maths::Max((u32)width, 1);
            u32 RenderPassHeight = Maths::Max((u32)height, 1);

            VkRenderPassBeginInfo rpBegin    = {};
            rpBegin.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpBegin.pNext                    = NULL;
            rpBegin.renderPass               = m_RenderPass;
            rpBegin.framebuffer              = static_cast<VKFramebuffer*>(frame)->GetFramebuffer();
            rpBegin.renderArea.offset.x      = 0;
            rpBegin.renderArea.offset.y      = 0;
            rpBegin.renderArea.extent.width  = RenderPassWidth;
            rpBegin.renderArea.extent.height = RenderPassHeight;
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
            commandBuffer->UpdateViewport(RenderPassWidth, RenderPassHeight, m_SwapchainTarget);

            s_ActiveCount++;
            Engine::Get().Statistics().BoundSceneRenderer++;
        }

        void VKRenderPass::EndRenderPass(CommandBuffer* commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION_LOW();

            ASSERT(s_ActiveCount, "No active SceneRenderer to end");
            s_ActiveCount--;
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
