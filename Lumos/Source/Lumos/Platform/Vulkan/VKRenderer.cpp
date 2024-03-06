#include "Precompiled.h"
#include "VKRenderer.h"
#include "VKDevice.h"
#include "VKShader.h"
#include "VKDescriptorSet.h"
#include "VKUtilities.h"
#include "VKPipeline.h"
#include "VKInitialisers.h"
#include "VKCommandBuffer.h"
#include "VKSwapChain.h"
#include "Core/Engine.h"
#include "Core/Application.h"
#include "Core/OS/Window.h"

#include "stb_image_write.h"

namespace Lumos
{
    namespace Graphics
    {
        static VkFence s_ComputeFence = nullptr;

        int VKRenderer::s_DeletionQueueIndex              = 0;
        Vector<DeletionQueue> VKRenderer::s_DeletionQueue = {};

        void VKRenderer::InitInternal()
        {
            LUMOS_PROFILE_FUNCTION();

            m_RendererTitle = "Vulkan";

            // Deletion queue larger than frames in flight to delay deletion a few frames
            s_DeletionQueue.Resize(12);
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
            subresourceRange.baseMipLevel            = 0;
            subresourceRange.layerCount              = 1;
            subresourceRange.levelCount              = 1;
            subresourceRange.baseArrayLayer          = 0;

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
            else if(texture->GetType() == TextureType::DEPTHARRAY)
            {
                VkImageLayout layout = ((VKTextureDepthArray*)texture)->GetImageLayout();

                VkClearDepthStencilValue clear_depth_stencil = { 1.0f, 1 };

                subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                subresourceRange.layerCount = static_cast<VKTextureDepthArray*>(texture)->GetCount();

                ((VKTextureDepthArray*)texture)->TransitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (VKCommandBuffer*)commandBuffer);
                vkCmdClearDepthStencilImage(((VKCommandBuffer*)commandBuffer)->GetHandle(), static_cast<VKTextureDepthArray*>(texture)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil, 1, &subresourceRange);
                ((VKTextureDepthArray*)texture)->TransitionImage(layout, (VKCommandBuffer*)commandBuffer);
            }
        }

        void VKRenderer::ClearSwapChainImage() const
        {
            LUMOS_PROFILE_FUNCTION_LOW();

            auto m_SwapChain = Application::Get().GetWindow()->GetSwapChain();
            for(int i = 0; i < m_SwapChain->GetSwapChainBufferCount(); i++)
            {
                auto cmd = VKUtilities::BeginSingleTimeCommands();

                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel            = 0;
                subresourceRange.layerCount              = 1;
                subresourceRange.levelCount              = 1;

                VkClearColorValue clearColourValue = VkClearColorValue({ { 0.0f, 0.0f, 0.0f, 0.0f } });

                vkCmdClearColorImage(cmd, static_cast<VKTexture2D*>(m_SwapChain->GetImage(i))->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &clearColourValue, 1, &subresourceRange);

                VKUtilities::EndSingleTimeCommands(cmd);
            }
        }

        void VKRenderer::OnResize(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(width == 0 || height == 0)
                return;

            LUMOS_LOG_INFO("VKRenderer::OnResize {0}, {1}", width, height);

            VKUtilities::ValidateResolution(width, height);
            Application::Get().GetWindow()->GetSwapChain().As<VKSwapChain>()->OnResize(width, height, true);
        }

        void VKRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            s_DeletionQueueIndex++;
            s_DeletionQueueIndex = s_DeletionQueueIndex % int(s_DeletionQueue.Size());
            s_DeletionQueue[s_DeletionQueueIndex].Flush();

            SharedPtr<VKSwapChain> swapChain = Application::Get().GetWindow()->GetSwapChain().As<VKSwapChain>();
            swapChain->Begin();
        }

        void VKRenderer::PresentInternal()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            SharedPtr<VKSwapChain> swapChain = Application::Get().GetWindow()->GetSwapChain().As<VKSwapChain>();

            swapChain->End();
            swapChain->QueueSubmit();

            auto& frameData = swapChain->GetCurrentFrameData();
            auto semaphore  = frameData.MainCommandBuffer->GetSemaphore();

            swapChain->Present(semaphore);
        }

        const std::string& VKRenderer::GetTitleInternal() const
        {
            return m_RendererTitle;
        }

        void VKRenderer::SaveScreenshot(const std::string& path, Graphics::Texture* texture)
        {
            bool supportsBlit = true;

            // Check blit support for source and destination
            VkFormatProperties formatProps;

            // Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
            vkGetPhysicalDeviceFormatProperties(VKDevice::Get().GetGPU(), ((VKTexture2D*)texture)->GetVKFormat(), &formatProps);
            if(!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
            {
                std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
                supportsBlit = false;
            }

            // Check if the device supports blitting to linear images
            vkGetPhysicalDeviceFormatProperties(VKDevice::Get().GetGPU(), VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
            if(!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
            {
                std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
                supportsBlit = false;
            }

            // Source for the copy is the last rendered swapchain image
            VkImage srcImage;

            if(texture)
                srcImage = ((VKTexture2D*)texture)->GetImage();
            else
                return;

            // Create the linear tiled destination image to copy to and to read the memory from
            VkImageCreateInfo imageCreateCI = VKInitialisers::ImageCreateInfo();
            imageCreateCI.imageType         = VK_IMAGE_TYPE_2D;
            // Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
            imageCreateCI.format        = VK_FORMAT_R8G8B8A8_UNORM;
            imageCreateCI.extent.width  = texture->GetWidth();
            imageCreateCI.extent.height = texture->GetHeight();
            imageCreateCI.extent.depth  = 1;
            imageCreateCI.arrayLayers   = 1;
            imageCreateCI.mipLevels     = 1;
            imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCreateCI.samples       = VK_SAMPLE_COUNT_1_BIT;
            imageCreateCI.tiling        = VK_IMAGE_TILING_LINEAR;
            imageCreateCI.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            // Create the image
            VkImage dstImage;
            VK_CHECK_RESULT(vkCreateImage(VKDevice::Get().GetDevice(), &imageCreateCI, nullptr, &dstImage));
            // Create memory to back up the image
            VkMemoryRequirements memRequirements;
            VkMemoryAllocateInfo memAllocInfo = VKInitialisers::MemoryAllocateInfo();
            VkDeviceMemory dstImageMemory;
            vkGetImageMemoryRequirements(VKDevice::Get().GetDevice(), dstImage, &memRequirements);
            memAllocInfo.allocationSize = memRequirements.size;
            // Memory must be host visible to copy from
            memAllocInfo.memoryTypeIndex = VKDevice::Get().GetPhysicalDevice()->GetMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            VK_CHECK_RESULT(vkAllocateMemory(VKDevice::Get().GetDevice(), &memAllocInfo, nullptr, &dstImageMemory));
            VK_CHECK_RESULT(vkBindImageMemory(VKDevice::Get().GetDevice(), dstImage, dstImageMemory, 0));

            // Do the actual blit from the swapchain image to our host visible destination image
            VkCommandBuffer copyCmd = VKUtilities::BeginSingleTimeCommands(); // vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

            // Transition destination image to transfer destination layout
            VKUtilities::InsertImageMemoryBarrier(
                copyCmd,
                dstImage,
                0,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

            // Transition swapchain image from present to transfer source layout
            VKUtilities::InsertImageMemoryBarrier(
                copyCmd,
                srcImage,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

            // If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
            if(supportsBlit)
            {
                // Define the region to blit (we will blit the whole swapchain image)
                VkOffset3D blitSize;
                blitSize.x = texture->GetWidth();
                blitSize.y = texture->GetHeight();
                blitSize.z = 1;
                VkImageBlit imageBlitRegion {};
                imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlitRegion.srcSubresource.layerCount = 1;
                imageBlitRegion.srcOffsets[1]             = blitSize;
                imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlitRegion.dstSubresource.layerCount = 1;
                imageBlitRegion.dstOffsets[1]             = blitSize;

                // Issue the blit command
                vkCmdBlitImage(
                    copyCmd,
                    srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageBlitRegion,
                    VK_FILTER_NEAREST);
            }
            else
            {
                // Otherwise use image copy (requires us to manually flip components)
                VkImageCopy imageCopyRegion {};
                imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageCopyRegion.srcSubresource.layerCount = 1;
                imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageCopyRegion.dstSubresource.layerCount = 1;
                imageCopyRegion.extent.width              = texture->GetWidth();
                imageCopyRegion.extent.height             = texture->GetHeight();
                imageCopyRegion.extent.depth              = 1;

                // Issue the copy command
                vkCmdCopyImage(
                    copyCmd,
                    srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &imageCopyRegion);
            }

            // Transition destination image to general layout, which is the required layout for mapping the image memory later on
            VKUtilities::InsertImageMemoryBarrier(
                copyCmd,
                dstImage,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

            // Transition back the swap chain image after the blit is done
            VKUtilities::InsertImageMemoryBarrier(
                copyCmd,
                srcImage,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

            VKUtilities::EndSingleTimeCommands(copyCmd);

            // Get layout of the image (including row pitch)
            VkImageSubresource subResource { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
            VkSubresourceLayout subResourceLayout;
            vkGetImageSubresourceLayout(VKDevice::Get().GetDevice(), dstImage, &subResource, &subResourceLayout);

            // Map image memory so we can start copying from it
            const char* data;
            vkMapMemory(VKDevice::Get().GetDevice(), dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
            data += subResourceLayout.offset;

            /*
std::ofstream file(path, std::ios::out | std::ios::binary);

// ppm header
file << "P6\n"
     << texture->GetWidth() << "\n"
     << texture->GetHeight() << "\n"
                    << 255 << "\n";

            */

            // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
            bool colorSwizzle = false;
            // Check if source is BGR
            // Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
            if(!supportsBlit)
            {
                std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
                colorSwizzle                     = (std::find(formatsBGR.begin(), formatsBGR.end(), ((VKTexture2D*)texture)->GetVKFormat()) != formatsBGR.end());
            }

            stbi_flip_vertically_on_write(1);

            uint32_t width  = texture->GetWidth();
            uint32_t height = texture->GetHeight();

            int32_t resWrite = stbi_write_png(
                path.c_str(),
                width,
                height,
                4,
                data,
                (int)subResourceLayout.rowPitch);
            /*

for(uint32_t y = 0; y < texture->GetHeight(); y++)
{
    unsigned int* row = (unsigned int*)data;
    for(uint32_t x = 0; x < texture->GetWidth(); x++)
    {
        if(colorSwizzle)
        {
            file.write((char*)row + 2, 1);
            file.write((char*)row + 1, 1);
            file.write((char*)row, 1);
        }
        else
        {
            file.write((char*)row, 3);
        }
        row++;
    }
    data += subResourceLayout.rowPitch;
}
file.close();
*/

            LUMOS_LOG_INFO("Screenshot saved to disk");

            // Clean up resources
            vkUnmapMemory(VKDevice::Get().GetDevice(), dstImageMemory);
            vkFreeMemory(VKDevice::Get().GetDevice(), dstImageMemory, nullptr);
            vkDestroyImage(VKDevice::Get().GetDevice(), dstImage, nullptr);
        }

        void VKRenderer::BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* commandBuffer, uint32_t dynamicOffset, Graphics::DescriptorSet** descriptorSets, uint32_t descriptorCount)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            uint32_t numDynamicDescriptorSets = 0;
            uint32_t numDesciptorSets         = 0;

            for(uint32_t i = 0; i < descriptorCount; i++)
            {
                if(descriptorSets[i])
                {
                    auto vkDesSet = static_cast<Graphics::VKDescriptorSet*>(descriptorSets[i]);
                    if(vkDesSet->GetIsDynamic())
                        numDynamicDescriptorSets++;

                    m_CurrentDescriptorSets[numDesciptorSets] = vkDesSet->GetDescriptorSet();

                    LUMOS_ASSERT(vkDesSet->GetHasUpdated(Renderer::GetMainSwapChain()->GetCurrentBufferIndex()), "Descriptor Set has not been updated before");
                    numDesciptorSets++;
                }
                else
                    LUMOS_LOG_ERROR("Descriptor null");
            }

            vkCmdBindDescriptorSets(static_cast<Graphics::VKCommandBuffer*>(commandBuffer)->GetHandle(), static_cast<Graphics::VKPipeline*>(pipeline)->IsCompute() ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), 0, numDesciptorSets, m_CurrentDescriptorSets, numDynamicDescriptorSets, &dynamicOffset);
        }

        void VKRenderer::DrawIndexedInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Engine::Get().Statistics().NumDrawCalls++;
            vkCmdDrawIndexed(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), count, 1, 0, 0, 0);
        }

        void VKRenderer::DrawInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, DataType datayType, void* indices) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Engine::Get().Statistics().NumDrawCalls++;
            vkCmdDraw(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), count, 1, 0, 0);
        }

        void VKRenderer::Dispatch(CommandBuffer* commandBuffer, uint32_t workGroupSizeX, uint32_t workGroupSizeY, uint32_t workGroupSizeZ)
        {
            VkCommandBuffer buffer = static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle();

            vkCmdDispatch(buffer, workGroupSizeX, workGroupSizeY, workGroupSizeZ);

            // commandBuffer->EndRecording();

            //            auto device = VKDevice::Get().GetDevice();
            //
            //            if (!s_ComputeFence)
            //            {
            //
            //                VkFenceCreateInfo fenceCreateInfo {};
            //                fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            // fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            //                VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &s_ComputeFence));
            //            }
            //
            //            vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
            //            vkResetFences(device, 1, &s_ComputeFence);
            //
            //            VkSubmitInfo computeSubmitInfo {};
            //            computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            //            computeSubmitInfo.commandBufferCount = 1;
            //            computeSubmitInfo.pCommandBuffers = &buffer;
            //            VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Get().GetComputeQueue(), 1, &computeSubmitInfo, s_ComputeFence));
            //
            //            // Wait for execution of compute shader to complete
            //            // Currently this is here for "safety"
            //            {
            //                vkWaitForFences(device, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX);
            //            }
        }

        void VKRenderer::DrawSplashScreen(Texture* texture)
        {
            LUMOS_PROFILE_FUNCTION();
            std::vector<TextureType> attachmentTypes;
            std::vector<Texture*> attachments;

            Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            auto image                             = Renderer::GetMainSwapChain()->GetCurrentImage();

            attachmentTypes.push_back(TextureType::COLOUR);
            attachments.push_back(image);

            Graphics::RenderPassDesc renderPassDesc;
            renderPassDesc.attachmentCount = uint32_t(attachmentTypes.size());
            renderPassDesc.attachmentTypes = attachmentTypes.data();
            renderPassDesc.attachments     = attachments.data();
            renderPassDesc.clear           = true;
            renderPassDesc.DebugName       = "Splash Screen Pass";

            float clearColour[4] = { 040.0f / 256.0f, 42.0f / 256.0f, 54.0f / 256.0f, 1.0f };

            int32_t width  = Application::Get().GetWindow()->GetWidth();
            int32_t height = Application::Get().GetWindow()->GetHeight();

            auto renderPass = Graphics::RenderPass::Get(renderPassDesc);

            FramebufferDesc frameBufferDesc {};
            frameBufferDesc.width           = width;
            frameBufferDesc.height          = height;
            frameBufferDesc.attachmentCount = uint32_t(attachments.size());
            frameBufferDesc.renderPass      = renderPass.get();
            frameBufferDesc.attachmentTypes = attachmentTypes.data();
            frameBufferDesc.attachments     = attachments.data();
            auto frameBuffer                = Framebuffer::Get(frameBufferDesc);

            // To clear screen
            renderPass->BeginRenderPass(commandBuffer, clearColour, frameBuffer, SubPassContents::INLINE, width, height);
            renderPass->EndRenderPass(commandBuffer);

            float ratio = float(texture->GetWidth() / texture->GetHeight());
            VkImageBlit blit {};
            blit.srcOffsets[0]                 = { 0, 0, 0 };
            blit.srcOffsets[1]                 = { (int32_t)texture->GetWidth(), (int32_t)texture->GetWidth(), 1 };
            blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel       = 0;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = 1;

            int32_t destSizex = width / 4;
            int32_t destSizey = int32_t(destSizex * ratio);
            int32_t offsetx   = width / 2 - destSizex / 2;
            int32_t offsety   = height / 2 - destSizey / 2;

            blit.dstOffsets[0]                 = { offsetx, offsety, 0 };
            blit.dstOffsets[1]                 = { offsetx + destSizex, offsety + destSizey, 1 };
            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel       = 0;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = 1;

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

        VkDescriptorPool VKRenderer::CreatePool(VkDevice device, uint32_t count, VkDescriptorPoolCreateFlags flags)
        {
            std::array<VkDescriptorPoolSize, 11> poolSizes = {
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER, count / 2 },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count * 4 },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, count },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, count },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, count },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, count },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count * 2 },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count * 2 },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, count },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, count },
                VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, count / 2 }
            };

            // Create info
            VkDescriptorPoolCreateInfo poolCreateInfo = {};
            poolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolCreateInfo.flags                      = flags;
            poolCreateInfo.poolSizeCount              = static_cast<uint32_t>(poolSizes.size());
            poolCreateInfo.pPoolSizes                 = poolSizes.data();
            poolCreateInfo.maxSets                    = count;

            VkDescriptorPool descriptorPool;
            vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool);

            return descriptorPool;
        }

        VkDescriptorPool VKRenderer::GetPool()
        {
            if(m_FreeDescriptorPools.Size() > 0)
            {
                VkDescriptorPool pool = m_FreeDescriptorPools.Back();
                m_FreeDescriptorPools.PopBack();
                return pool;
            }
            else
            {
                return CreatePool(VKDevice::Get().GetDevice(), 100, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
            }
        }

        bool VKRenderer::AllocateDescriptorSet(VkDescriptorSet* set, VkDescriptorSetLayout layout, uint32_t descriptorCount)
        {
            if(m_CurrentPool == VK_NULL_HANDLE)
            {
                m_CurrentPool = GetPool();
                m_UsedDescriptorPools.PushBack(m_CurrentPool);
            }

            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext                       = nullptr;
            allocInfo.pSetLayouts                 = &layout;
            allocInfo.descriptorPool              = m_CurrentPool;
            allocInfo.descriptorSetCount          = descriptorCount;

            VkResult allocResult = vkAllocateDescriptorSets(VKDevice::Get().GetDevice(), &allocInfo, set);
            bool needReallocate  = false;

            switch(allocResult)
            {
            case VK_SUCCESS:
                // all good, return
                return true;

                break;
            case VK_ERROR_FRAGMENTED_POOL:
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                // reallocate pool
                needReallocate = true;
                break;
            default:
                // unrecoverable error
                return false;
            }

            if(needReallocate)
            {
                // allocate a new pool and retry
                m_CurrentPool = GetPool();
                m_UsedDescriptorPools.PushBack(m_CurrentPool);
                allocInfo.descriptorPool = m_CurrentPool;

                allocResult = vkAllocateDescriptorSets(VKDevice::Get().GetDevice(), &allocInfo, set);

                // if it still fails then we have big issues
                if(allocResult == VK_SUCCESS)
                {
                    return true;
                }
            }

            return false;
        }

        void VKRenderer::ReleaseDescriptorPools()
        {
            for(auto p : m_FreeDescriptorPools)
            {
                vkDestroyDescriptorPool(VKDevice::Get().GetDevice(), p, nullptr);
            }
            for(auto p : m_UsedDescriptorPools)
            {
                vkDestroyDescriptorPool(VKDevice::Get().GetDevice(), p, nullptr);
            }
        }

        RHIFormat VKRenderer::GetDepthFormat()
        {
            VkFormat depthFormat = VKUtilities::FindDepthFormat();
            return VKUtilities::VKToFormat(depthFormat);
        }
    }
}
