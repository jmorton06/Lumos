#include "LM.h"
#include "VKRenderer.h"
#include "VKDevice.h"
#include "Graphics/MeshFactory.h"
#include "VKShader.h"
#include "Graphics/Model/Model.h"
#include "VKInitialisers.h"
#include "System/System.h"
#include "App/Scene.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VKDescriptorSet.h"
#include "App/SceneManager.h"

namespace Lumos
{
	namespace graphics
	{
		void VKRenderer::InitInternal()
		{
			m_Context = VKContext::Get();

			m_RendererTitle = "Vulkan";

            VKDevice::Instance();
            
            m_Swapchain = new VKSwapchain(m_Width, m_Height);
            m_Swapchain->Init();
            
            CreateSemaphores();
		}

        VKRenderer::~VKRenderer()
		{
            delete m_Swapchain;
            
            vkDestroySemaphore(VKDevice::Instance()->GetDevice(), renderFinishedSemaphore, nullptr);
            vkDestroySemaphore(VKDevice::Instance()->GetDevice(), imageAvailableSemaphore, nullptr);
            
            m_Context->Unload();
		}

		void VKRenderer::PresentInternal(api::CommandBuffer* cmdBuffer)
		{
			auto result = m_Swapchain->AcquireNextImage(imageAvailableSemaphore);

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				OnResize(m_Width,m_Height);
				return;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			{
				throw std::runtime_error("failed to acquire swap chain image!");
			}

			((VKCommandBuffer*)cmdBuffer)->ExecuteInternal(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				imageAvailableSemaphore, renderFinishedSemaphore, false);
				//SetPreviousImageAvailable(m_PreviousRenderFinishedSemaphore);
                //SetPreviousRenderFinish(m_PreviousImageAvailableSemaphore);
		}
        
        void VKRenderer::PresentInternal()
        {
            m_Swapchain->Present(renderFinishedSemaphore);
            VK_CHECK_RESULT(vkQueueWaitIdle(VKDevice::Instance()->GetPresentQueue()));
        }

		void VKRenderer::OnResize(uint width, uint height)
		{
			if (width == 0 || height == 0) return;

			m_Width  = width;
			m_Height = height;

			delete m_Swapchain;
			m_Swapchain = new VKSwapchain(width, height);
			m_Swapchain->Init();
		}

		void VKRenderer::CreateSemaphores()
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			if (vkCreateSemaphore(VKDevice::Instance()->GetDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) !=
				VK_SUCCESS ||
				vkCreateSemaphore(VKDevice::Instance()->GetDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) !=
				VK_SUCCESS)
			{
				throw std::runtime_error("failed to create semaphores!");
			}

            //m_PreviousImageAvailableSemaphore = imageAvailableSemaphore;
            //m_PreviousRenderFinishedSemaphore = renderFinishedSemaphore;
		}

		void VKRenderer::Render(IndexBuffer* indexBuffer, VertexArray* vertexArray,
			VKCommandBuffer* commandBuffer, std::vector<VkDescriptorSet>& descriptorSet, VkPipelineLayout layout, uint32_t offset, uint numDynamicDescriptorSets)
		{
			uint vertexArraySize = vertexArray->GetCount();
			VkBuffer* vertexBuffers = new VkBuffer[vertexArraySize];
			VkDeviceSize* offsets = new VkDeviceSize[vertexArraySize];
			for (uint i = 0; i < vertexArraySize; i++)
			{
				VKVertexBuffer* buffer = static_cast<VKVertexBuffer*>(vertexArray->GetBuffer(i));
				vertexBuffers[i] = buffer->GetBuffer();
				offsets[i] = static_cast<VkDeviceSize>(buffer->GetLayout().GetStride());
			}

			vkCmdBindDescriptorSets(commandBuffer->GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, static_cast<uint32_t>(descriptorSet.size()), descriptorSet.data(), numDynamicDescriptorSets, &offset);
			vkCmdBindVertexBuffers(commandBuffer->GetCommandBuffer(), 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer->GetCommandBuffer(), static_cast<VKIndexBuffer*>(indexBuffer)->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer->GetCommandBuffer(), static_cast<uint32_t>(indexBuffer->GetCount()), 1, 0, 0, 0);

			delete[] vertexBuffers;
			delete[] offsets;
		}

		void VKRenderer::Begin()
		{
		}
		void VKRenderer::BindScreenFBOInternal()
		{
		}
		void VKRenderer::ClearInternal(uint buffer)
		{
		}
		void VKRenderer::SetColourMaskInternal(bool r, bool g, bool b, bool a)
		{
		}
		void VKRenderer::SetDepthTestingInternal(bool enabled)
		{
		}
		void VKRenderer::SetStencilTestInternal(bool enabled)
		{
		}
		void VKRenderer::SetCullingInternal(bool enabled, bool front)
		{
		}
		void VKRenderer::SetBlendInternal(bool enabled)
		{
		}
		void VKRenderer::SetDepthMaskInternal(bool enabled)
		{
		}
		void VKRenderer::SetViewportInternal(uint x, uint y, uint width, uint height)
		{
		}
		void VKRenderer::SetBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination)
		{
		}
		void VKRenderer::SetBlendEquationInternal(RendererBlendFunction blendEquation)
		{
		}
		void VKRenderer::SetStencilFunctionInternal(StencilType type, uint ref, uint mask)
		{
		}
		void VKRenderer::SetStencilOpInternal(StencilType fail, StencilType zfail, StencilType zpass)
		{
		}
		const String & VKRenderer::GetTitleInternal() const
		{
			return m_RendererTitle;
		}
		void VKRenderer::DrawArraysInternal(DrawType type, uint numIndices) const
		{
		}
		void VKRenderer::DrawArraysInternal(DrawType type, uint start, uint numIndices) const
		{
		}
		void VKRenderer::DrawInternal(DrawType type, uint count, DataType datayType, void * indices) const
		{
		}
		void VKRenderer::SetRenderTargets(uint numTargets)
		{
		}
		void VKRenderer::SetPixelPackType(PixelPackType type)
		{
		}
		void VKRenderer::SetRenderModeInternal(RenderMode mode)
		{
		}

		void VKRenderer::RenderMeshInternal(Mesh *mesh, graphics::api::Pipeline *pipeline, graphics::api::CommandBuffer* cmdBuffer, uint dynamicOffset, graphics::api::DescriptorSet* descriptorSet, bool useMaterialDescriptorSet)
		{
			std::vector<VkDescriptorSet> descriptorSets;
			uint numDynamicDescriptorSets = 0;

			if (static_cast<graphics::VKDescriptorSet*>(pipeline->GetDescriptorSet())->GetIsDynamic())
				numDynamicDescriptorSets++;

			descriptorSets.push_back(static_cast<graphics::VKDescriptorSet*>(pipeline->GetDescriptorSet())->GetDescriptorSet());
			if(useMaterialDescriptorSet)
			{
				if (mesh->GetMaterial() && mesh->GetMaterial()->GetDescriptorSet())
				{
					descriptorSets.push_back(static_cast<graphics::VKDescriptorSet*>(mesh->GetMaterial()->GetDescriptorSet())->GetDescriptorSet());
					if (static_cast<graphics::VKDescriptorSet*>(mesh->GetMaterial()->GetDescriptorSet())->GetIsDynamic())
						numDynamicDescriptorSets++;
				}
				else if(descriptorSet)
				{
					descriptorSets.push_back(static_cast<graphics::VKDescriptorSet*>(descriptorSet)->GetDescriptorSet());
					if (static_cast<graphics::VKDescriptorSet*>(descriptorSet)->GetIsDynamic())
						numDynamicDescriptorSets++;
				}
			}

			uint index = 0;
			for(auto pc : static_cast<graphics::VKDescriptorSet*>(pipeline->GetDescriptorSet())->GetPushConstants())
			{
				//TODO : Shader Stage;
				vkCmdPushConstants(static_cast<graphics::VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer(), static_cast<graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, index, pc.size, pc.data);
			}


			graphics::VKRenderer::Render(mesh->GetIndexBuffer().get(), mesh->GetVertexArray().get(), static_cast<graphics::VKCommandBuffer*>(cmdBuffer), descriptorSets, static_cast<graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), dynamicOffset, numDynamicDescriptorSets);

		}
	}
}
