#pragma once

#include "VK.h"
#include "VKContext.h"
#include "VKSwapChain.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Graphics/RHI/RenderPass.h"
#include "VKUniformBuffer.h"
#include "VKDescriptorSet.h"
#include "Graphics/RHI/Renderer.h"
#include "Core/DataStructures/TDArray.h"
#include "Utilities/DeletionQueue.h"

#define NUM_SEMAPHORES 10

namespace Lumos
{
    namespace Graphics
    {
        class CommandBuffer;

        class LUMOS_EXPORT VKRenderer : public Renderer
        {
        public:
            VKRenderer() = default;
            ~VKRenderer();

            static VKRenderer* GetRenderer()
            {
                return static_cast<VKRenderer*>(s_Instance);
            }

            static VKContext* GetGraphicsContext()
            {
                return static_cast<VKContext*>(s_Instance->GetGraphicsContext());
            }

            static VKSwapChain* GetMainSwapChain()
            {
                return static_cast<VKSwapChain*>(Renderer::GetMainSwapChain());
            }

            void InitInternal() override;
            void Begin() override;
            void OnResize(uint32_t width, uint32_t height) override;

            void PresentInternal() override;
            void PresentInternal(CommandBuffer* commandBuffer) override;

            void ClearRenderTarget(Graphics::Texture* texture, Graphics::CommandBuffer* commandBuffer, Vec4 clearColour) override;
            void ClearSwapChainImage() const;

            void SaveScreenshot(const std::string& path, Graphics::Texture* texture = nullptr) override;

            const char* GetTitleInternal() const override;

            void BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* commandBuffer, uint32_t dynamicOffset, Graphics::DescriptorSet** descriptorSets, uint32_t descriptorCount) override;
            void DrawIndexedInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start) const override;
            void DrawInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, DataType datayType, void* indices) const override;
            void DrawSplashScreen(Texture* texture) override;
            uint32_t GetGPUCount() const override;
            bool SupportsCompute() override { return true; }
            void Dispatch(CommandBuffer* commandBuffer, uint32_t workGroupSizeX, uint32_t workGroupSizeY, uint32_t workGroupSizeZ) override;

            bool AllocateDescriptorSet(VkDescriptorSet* set, VkDescriptorPool& pool, VkDescriptorSetLayout layout, uint32_t descriptorCount);
            bool DeallocateDescriptorSet(VkDescriptorSet* set, VkDescriptorPool& pool);
            void ReleaseDescriptorPools();

            static DeletionQueue& GetDeletionQueue(int frameIndex)
            {
                ASSERT(frameIndex < int(s_DeletionQueue.Size()), "Unsupported Frame Index");
                return s_DeletionQueue[frameIndex];
            }

            static DeletionQueue& GetCurrentDeletionQueue()
            {
                return s_DeletionQueue[s_DeletionQueueIndex];
            }

            static void FlushDeletionQueues()
            {
                for(auto& deletionQueue : s_DeletionQueue)
                    deletionQueue.Flush();
            }

            template <typename F>
            static void PushDeletionFunction(F&& function)
            {
                GetCurrentDeletionQueue().PushFunction(function);
            }

            RHIFormat GetDepthFormat() override;

            static void MakeDefault();

        protected:
            static Renderer* CreateFuncVulkan();

        private:
            VkDescriptorPool CreatePool(VkDevice device, uint32_t count, VkDescriptorPoolCreateFlags flags);
            VkDescriptorPool GetPool();

            uint32_t m_CurrentSemaphoreIndex = 0;

            const char* m_RendererTitle;

            VkDescriptorPool m_CurrentPool;

            TDArray<u32> m_UsedDescriptorPoolsCapacity;
            TDArray<VkDescriptorPool> m_UsedDescriptorPools;
            TDArray<VkDescriptorPool> m_FreeDescriptorPools;

            static TDArray<DeletionQueue> s_DeletionQueue;
            static int s_DeletionQueueIndex;
        };
    }
}
