#pragma once

#include "JM.h"
#include "Dependencies/vulkan/vulkan.h"

#include "Maths/Maths.h"
#include "VKContext.h"
#include "VKTools.h"
#include "Graphics/Mesh.h"
#include "VKSwapchain.h"
#include "VKPipeline.h"
#include "Graphics/API/DescriptorSet.h"
#include "Graphics/API/RenderPass.h"
#include "VKUniformBuffer.h"
#include "VKDescriptorSet.h"

#include "Graphics/API/Renderer.h"

namespace jm
{
	namespace graphics
	{
		struct Vertex
		{
			jm::maths::Vector3 pos;
			jm::maths::Vector3 color;
			jm::maths::Vector2 texCoord;
			jm::maths::Vector3 normal;
			jm::maths::Vector3 tangent;

			static std::array<api::VertexInputDescription, 5> getAttributeDescriptions()
			{
				std::array<api::VertexInputDescription, 5> attributeDescriptions = {};

				attributeDescriptions[0].binding = 0;
				attributeDescriptions[0].location = 0;
				attributeDescriptions[0].format = api::Format::R32G32B32_FLOAT;
				attributeDescriptions[0].offset = offsetof(Vertex, pos);

				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = api::Format::R32G32B32_FLOAT;
				attributeDescriptions[1].offset = offsetof(Vertex, color);

				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = api::Format::R32G32_FLOAT;
				attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

				attributeDescriptions[3].binding = 0;
				attributeDescriptions[3].location = 3;
				attributeDescriptions[3].format = api::Format::R32G32B32_FLOAT;
				attributeDescriptions[3].offset = offsetof(Vertex, normal);

				attributeDescriptions[4].binding = 0;
				attributeDescriptions[4].location = 4;
				attributeDescriptions[4].format = api::Format::R32G32B32_FLOAT;
				attributeDescriptions[4].offset = offsetof(Vertex, tangent);

				return attributeDescriptions;
			}
		};

		class JM_EXPORT VKRenderer : public Renderer
		{
		public:
            VKRenderer(uint width, uint height) { m_Width = width; m_Height = height; }

			void cleanup();

			static VKRenderer* GetRenderer() { return static_cast<VKRenderer*>(s_Instance); }

			static void Render(IndexBuffer* indexBuffer, VertexArray* vertexBuffer, VKCommandBuffer* commandBuffer, std::vector<VkDescriptorSet>& descriptorSet, VkPipelineLayout layout, uint32_t offset, uint numDynamicDescriptorSets);
			void Present(VKCommandBuffer* cmdBuffer);

			api::Swapchain* GetSwapchainInternal() const override { return m_Swapchain; }

			void InitInternal() override;
			void Begin() override;
			void BindScreenFBOInternal() override;
			void OnResize(uint width, uint height) override;

			void ClearInternal(uint buffer) override;
			void PresentInternal() override;

			void SetColourMaskInternal(bool r, bool g, bool b, bool a) override;
			void SetDepthTestingInternal(bool enabled) override;
			void SetStencilTestInternal(bool enabled) override;
			void SetCullingInternal(bool enabled, bool front) override;
			void SetBlendInternal(bool enabled) override;
			void SetDepthMaskInternal(bool enabled) override;
			void SetViewportInternal(uint x, uint y, uint width, uint height) override;

			void SetBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination) override;
			void SetBlendEquationInternal(RendererBlendFunction blendEquation) override;
			void SetStencilFunctionInternal(StencilType type, uint ref, uint mask) override;
			void SetStencilOpInternal(StencilType fail, StencilType zfail, StencilType zpass) override;

			const String& GetTitleInternal() const override;
			void DrawArraysInternal(DrawType type, uint numIndices) const override;
			void DrawArraysInternal(DrawType type, uint start, uint numIndices) const override;
			void DrawInternal(DrawType type, uint count, DataType datayType, void* indices) const override;
			void SetRenderTargets(uint numTargets) override;
			void SetPixelPackType(PixelPackType type) override;
			void SetRenderModeInternal(RenderMode mode) override;

			void RenderMeshInternal(Mesh* mesh, graphics::api::Pipeline* pipeline, graphics::api::CommandBuffer* cmdBuffer, uint dynamicOffset, graphics::api::DescriptorSet* descriptorSet, bool useMaterialDescriptorSet) override;

		private:
			jm::graphics::VKContext* m_Context;

			jm::graphics::VKSwapchain* m_Swapchain;

			VkSemaphore imageAvailableSemaphore;
			VkSemaphore renderFinishedSemaphore;

			String m_RendererTitle;
			uint m_Width, m_Height;

			void initVulkan();

			void createSemaphores();
		};
	}
}
