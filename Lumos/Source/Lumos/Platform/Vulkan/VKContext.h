#pragma once
#include "Graphics/RHI/GraphicsContext.h"
#include "Core/Reference.h"
#include "VK.h"
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKCommandPool;

        class VKContext : public GraphicsContext
        {
        public:
            VKContext();
            ~VKContext();

            void Init() override;
            void Present() override;

            static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags,
                                                                VkDebugReportObjectTypeEXT objType,
                                                                uint64_t sourceObj,
                                                                size_t location,
                                                                int32_t msgCode,
                                                                const char* pLayerPrefix,
                                                                const char* pMsg,
                                                                void* userData);

            static VkInstance GetVKInstance() { return s_VkInstance; }

            size_t GetMinUniformBufferOffsetAlignment() const override;

            bool FlipImGUITexture() const override { return true; }
            void WaitIdle() const override;
            void OnImGui() override;

            float GetGPUMemoryUsed() override { return 0.0f; };
            float GetTotalGPUMemory() override { return 0.0f; };

            const TDArray<const char*>& GetLayerNames() const { return m_InstanceLayerNames; }
            const TDArray<const char*>& GetExtensionNames() const { return m_InstanceExtensionNames; }

            static bool ValidationEnabled() { return s_ValidationEnabled; }

            static void MakeDefault();
            static uint32_t GetVKVersion() { return m_VKVersion; }

        protected:
            static GraphicsContext* CreateFuncVulkan();

            void CreateInstance();
            void SetupDebugCallback();

            const TDArray<const char*> GetRequiredExtensions(bool enableValidationLayers);
            const TDArray<const char*> GetRequiredLayers(bool enableValidationLayers);

        private:
            static VkInstance s_VkInstance;
            VkDebugReportCallbackEXT m_DebugCallback = VK_NULL_HANDLE;

            TDArray<VkLayerProperties> m_InstanceLayers;
            TDArray<VkExtensionProperties> m_InstanceExtensions;

            TDArray<const char*> m_InstanceLayerNames;
            TDArray<const char*> m_InstanceExtensionNames;

            static bool s_ValidationEnabled;

            static uint32_t m_VKVersion;
        };
    }
}
