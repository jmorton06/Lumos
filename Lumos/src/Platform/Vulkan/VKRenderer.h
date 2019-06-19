#pragma once

#include "LM.h"
#include "VK.h"
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

namespace Lumos
{
	namespace Graphics
	{
		class CommandBuffer;

		class LUMOS_EXPORT VKRenderer : public Renderer
		{
		public:
            VKRenderer(uint width, uint height) { m_Width = width; m_Height = height; }
            ~VKRenderer();

			static VKRenderer* GetRenderer() { return static_cast<VKRenderer*>(s_Instance); }

			static void Render(IndexBuffer* indexBuffer, VertexArray* vertexBuffer, VKCommandBuffer* commandBuffer, std::vector<vk::DescriptorSet>& descriptorSet, vk::PipelineLayout layout, uint32_t offset, uint numDynamicDescriptorSets);

			Swapchain* GetSwapchainInternal() const override { return m_Swapchain; }

			void InitInternal() override;
			void Begin() override;
			void BindScreenFBOInternal() override;
			void OnResize(uint width, uint height) override;

			void ClearInternal(uint buffer) override;
			void PresentInternal() override;
			void PresentInternal(CommandBuffer* cmdBuffer) override;

			void ClearSwapchainImage() const;

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

			void RenderMeshInternal(Mesh* mesh, Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, uint dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets) override;
			void Render(VertexArray* vertexArray, IndexBuffer* indexBuffer, Graphics::CommandBuffer* cmdBuffer, std::vector<Graphics::DescriptorSet*>& descriptorSets, Graphics::Pipeline* pipeline, uint dynamicOffset) override;
            void CreateSemaphores();

		private:
			Lumos::Graphics::VKContext* m_Context;

			Lumos::Graphics::VKSwapchain* m_Swapchain;

			vk::Semaphore m_ImageAvailableSemaphore[5];
			uint m_CurrentSemaphoreIndex = 0;

			String m_RendererTitle;
			uint m_Width, m_Height;
		};
	}
}
