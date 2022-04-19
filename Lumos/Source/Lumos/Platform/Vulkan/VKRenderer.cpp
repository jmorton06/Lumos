#include "Precompiled.h"
#include "VKRenderer.h"
#include "VKDevice.h"
#include "VKShader.h"
#include "VKDescriptorSet.h"
#include "VKUtilities.h"
#include "VKPipeline.h"
#include "VKCommandBuffer.h"
#include "Core/Engine.h"
#include "Core/Application.h"

namespace Lumos
{
    namespace Graphics
    {
        static constexpr uint32_t MAX_DESCRIPTOR_SET_COUNT = 1500;
        VKContext::DeletionQueue VKRenderer::s_DeletionQueue[3] = {};
        VkDescriptorPool VKRenderer::s_DescriptorPool = {};

        void VKRenderer::InitInternal()
        {
            LUMOS_PROFILE_FUNCTION();

            m_RendererTitle = "Vulkan";

            // Pool sizes
            VkDescriptorPoolSize pool_sizes[] = {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            // Create info
            VkDescriptorPoolCreateInfo pool_create_info = {};
            pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_create_info.poolSizeCount = 11;
            pool_create_info.pPoolSizes = pool_sizes;
            pool_create_info.maxSets = MAX_DESCRIPTOR_SET_COUNT;

            // Pool
            VK_CHECK_RESULT(vkCreateDescriptorPool(VKDevice::Get().GetDevice(), &pool_create_info, nullptr, &s_DescriptorPool));
        }

        VKRenderer::~VKRenderer()
        {
            // DescriptorPool deleted by VKContext
        }

        void VKRenderer::PresentInternal(CommandBuffer* commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION();
        }

        void VKRenderer::ClearRenderTarget(Graphics::Texture* texture, Graphics::CommandBuffer* commandBuffer, glm::vec4 clearColour)
        {
            VkImageSubresourceRange subresourceRange = {}; // TODO: Get from texture
            subresourceRange.baseMipLevel = 0;
            subresourceRange.layerCount = 1;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;

            if(texture->GetType() == TextureType::COLOUR)
            {
                VkImageLayout layout = ((VKTexture2D*)texture)->GetImageLayout();
                ((VKTexture2D*)texture)->TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (VKCommandBuffer*)commandBuffer);

                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

                VkClearColorValue clearColourValue = VkClearColorValue({ { clearColour.x, clearColour.y, clearColour.z, clearColour.w } });
                vkCmdClearColorImage(((VKCommandBuffer*)commandBuffer)->GetHandle(), static_cast<VKTexture2D*>(texture)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColourValue, 1, &subresourceRange);
                ((VKTexture2D*)texture)->TransitionImage(layout, (VKCommandBuffer*)commandBuffer);
            }
            else if(texture->GetType() == TextureType::DEPTH)
            {
                VkClearDepthStencilValue clear_depth_stencil = { 1.0f, 1 };

                subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                ((VKTextureDepth*)texture)->TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (VKCommandBuffer*)commandBuffer);
                vkCmdClearDepthStencilImage(((VKCommandBuffer*)commandBuffer)->GetHandle(), static_cast<VKTextureDepth*>(texture)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil, 1, &subresourceRange);
            }
        }

        void VKRenderer::ClearSwapChainImage() const
        {
            LUMOS_PROFILE_FUNCTION();

            auto m_SwapChain = Application::Get().GetWindow()->GetSwapChain();
            for(int i = 0; i < m_SwapChain->GetSwapChainBufferCount(); i++)
            {
                auto cmd = VKUtilities::BeginSingleTimeCommands();

                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel = 0;
                subresourceRange.layerCount = 1;
                subresourceRange.levelCount = 1;

                VkClearColorValue clearColourValue = VkClearColorValue({ { 0.0f, 0.0f, 0.0f, 0.0f } });

                vkCmdClearColorImage(cmd, static_cast<VKTexture2D*>(m_SwapChain->GetImage(i))->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &clearColourValue, 1, &subresourceRange);

                VKUtilities::EndSingleTimeCommands(cmd);
            }
        }

        void VKRenderer::OnResize(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
            if(width == 0 || height == 0)
                return;

            VKUtilities::ValidateResolution(width, height);
            Application::Get().GetWindow()->GetSwapChain().As<VKSwapChain>()->OnResize(width, height);
        }

        void VKRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
            SharedPtr<VKSwapChain> swapChain = Application::Get().GetWindow()->GetSwapChain().As<VKSwapChain>();
            swapChain->AcquireNextImage();
            swapChain->Begin();
        }

        void VKRenderer::PresentInternal()
        {
            LUMOS_PROFILE_FUNCTION();
            SharedPtr<VKSwapChain> swapChain = Application::Get().GetWindow()->GetSwapChain().As<VKSwapChain>();

            swapChain->End();
            swapChain->QueueSubmit();

            auto& frameData = swapChain->GetCurrentFrameData();
            auto semphore = frameData.MainCommandBuffer->GetSemaphore();

            swapChain->Present(semphore);
        }

        const std::string& VKRenderer::GetTitleInternal() const
        {
            return m_RendererTitle;
        }

        void VKRenderer::BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* commandBuffer, uint32_t dynamicOffset, Graphics::DescriptorSet** descriptorSets, uint32_t descriptorCount)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t numDynamicDescriptorSets = 0;
            uint32_t numDesciptorSets = 0;

            for(uint32_t i = 0; i < descriptorCount; i++)
            {
                if(descriptorSets[i])
                {
                    auto vkDesSet = static_cast<Graphics::VKDescriptorSet*>(descriptorSets[i]);
                    if(vkDesSet->GetIsDynamic())
                        numDynamicDescriptorSets++;

                    m_DescriptorSetPool[numDesciptorSets] = vkDesSet->GetDescriptorSet();

                    LUMOS_ASSERT(vkDesSet->GetHasUpdated(Renderer::GetMainSwapChain()->GetCurrentBufferIndex()), "Descriptor Set has not been updated before");
                    numDesciptorSets++;
                }
            }

            vkCmdBindDescriptorSets(static_cast<Graphics::VKCommandBuffer*>(commandBuffer)->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), 0, numDesciptorSets, m_DescriptorSetPool, numDynamicDescriptorSets, &dynamicOffset);
        }

        void VKRenderer::DrawIndexedInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start) const
        {
            LUMOS_PROFILE_FUNCTION();
            Engine::Get().Statistics().NumDrawCalls++;
            vkCmdDrawIndexed(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), count, 1, 0, 0, 0);
        }

        void VKRenderer::DrawInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, DataType datayType, void* indices) const
        {
            LUMOS_PROFILE_FUNCTION();
            Engine::Get().Statistics().NumDrawCalls++;
            vkCmdDraw(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), count, 1, 0, 0);
        }

        void VKRenderer::DrawSplashScreen(Texture* texture)
        {
            std::vector<TextureType> attachmentTypes;
            std::vector<Texture*> attachments;

            Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            auto image = Renderer::GetMainSwapChain()->GetCurrentImage();

            attachmentTypes.push_back(TextureType::COLOUR);
            attachments.push_back(image);

            Graphics::RenderPassDesc renderPassDesc;
            renderPassDesc.attachmentCount = uint32_t(attachmentTypes.size());
            renderPassDesc.attachmentTypes = attachmentTypes.data();
            renderPassDesc.attachments = attachments.data();
            renderPassDesc.clear = true;

            glm::vec4 clearColour = glm::vec4(040.0f / 256.0f, 42.0f / 256.0f, 54.0f / 256.0f, 1.0f);

            int32_t width = Application::Get().GetWindow()->GetWidth();
            int32_t height = Application::Get().GetWindow()->GetHeight();

            auto renderPass = Graphics::RenderPass::Get(renderPassDesc);

            FramebufferDesc frameBufferDesc {};
            frameBufferDesc.width = width;
            frameBufferDesc.height = height;
            frameBufferDesc.attachmentCount = uint32_t(attachments.size());
            frameBufferDesc.renderPass = renderPass.get();
            frameBufferDesc.attachmentTypes = attachmentTypes.data();
            frameBufferDesc.attachments = attachments.data();
            auto frameBuffer = Framebuffer::Get(frameBufferDesc);

            // To clear screen
            renderPass->BeginRenderpass(commandBuffer, clearColour, frameBuffer, SubPassContents::INLINE, width, height);
            renderPass->EndRenderpass(commandBuffer);

            float ratio = float(texture->GetWidth() / texture->GetHeight());
            VkImageBlit blit {};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { (int32_t)texture->GetWidth(), (int32_t)texture->GetWidth(), 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = 0;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;

            int32_t destSizex = width / 4;
            int32_t destSizey = int32_t(destSizex * ratio);
            int32_t offsetx = width / 2 - destSizex / 2;
            int32_t offsety = height / 2 - destSizey / 2;

            blit.dstOffsets[0] = { offsetx, offsety, 0 };
            blit.dstOffsets[1] = { offsetx + destSizex, offsety + destSizey, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = 0;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            VkImageLayout layout = ((VKTexture2D*)image)->GetImageLayout();

            ((VKTexture2D*)texture)->TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, (VKCommandBuffer*)commandBuffer);
            ((VKTexture2D*)image)->TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (VKCommandBuffer*)commandBuffer);

            vkCmdBlitImage(((VKCommandBuffer*)commandBuffer)->GetHandle(),
                ((VKTexture2D*)texture)->GetImage(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                ((VKTexture2D*)image)->GetImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &blit,
                VK_FILTER_LINEAR);

            ((VKTexture2D*)image)->TransitionImage(layout, (VKCommandBuffer*)commandBuffer);
        }

        void VKRenderer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        Renderer* VKRenderer::CreateFuncVulkan()
        {
            return new VKRenderer();
        }

        uint32_t VKRenderer::GetGPUCount() const
        {
            return VKDevice::Get().GetGPUCount();
        }
    }
}
