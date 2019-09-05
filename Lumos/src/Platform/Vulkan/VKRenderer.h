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
            VKRenderer(u32 width, u32 height) { m_Width = width; m_Height = height; }
            ~VKRenderer();

			static VKRenderer* GetRenderer() { return static_cast<VKRenderer*>(s_Instance); }

			Swapchain* GetSwapchainInternal() const override { return m_Swapchain.get(); }

			void InitInternal() override;
			void Begin() override;
			void OnResize(u32 width, u32 height) override;

			void PresentInternal() override;
			void PresentInternal(CommandBuffer* cmdBuffer) override;

			void ClearSwapchainImage() const;

			const String& GetTitleInternal() const override;

			void BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets) override;
			void DrawIndexedInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, u32 start) const override;
			void DrawInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, DataType datayType, void* indices) const override;

            void CreateSemaphores();
            
            static void MakeDefault();
        protected:
            static Renderer* CreateFuncVulkan(u32 width, u32 height);
		private:
			Lumos::Graphics::VKContext* m_Context;
			Ref<Lumos::Graphics::VKSwapchain> m_Swapchain;

			vk::Semaphore m_ImageAvailableSemaphore[5];
			u32 m_CurrentSemaphoreIndex = 0;

			String m_RendererTitle;
			u32 m_Width, m_Height;
		};
	}
}
