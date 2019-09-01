#pragma once
#include "Graphics/API/GraphicsContext.h"

#include "VK.h"

#ifdef USE_VMA_ALLOCATOR
#include <vulkan/vk_mem_alloc.h>
#endif

#ifdef LUMOS_DEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = false;
#endif

namespace Lumos
{
	namespace Graphics
	{
		class VKCommandPool;

		class VKContext : public GraphicsContext
		{
		public:
			VKContext(const WindowProperties& properties, void* deviceContext);
			~VKContext();

			void Init() override;
			void Present() override;
			void Unload();

			inline static VKContext* Get() { return static_cast<VKContext*>(s_Context); }

			static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags,
				VkDebugReportObjectTypeEXT objType,
				uint64_t sourceObj,
				size_t location,
				int32_t msgCode,
				const char* pLayerPrefix,
				const char* pMsg,
				void* userData);

			vk::Instance GetVKInstance()		const { return m_VkInstance; }
			VKCommandPool* GetCommandPool()		const { return m_CommandPool; }
			void* GetWindowContext()			const { return m_WindowContext; }

			size_t GetMinUniformBufferOffsetAlignment() const override;

            bool FlipImGUITexture() const override { return false; }
			void OnImGUI() override;

			const std::vector<const char*>& GetLayerNames()			const { return m_InstanceLayerNames; }
			const std::vector<const char*>& GetExtensionNames()		const { return m_InstanceExtensionNames; }

            static void MakeDefault();
        protected:
            static GraphicsContext* CreateFuncVulkan();

			void CreateInstance();
			void SetupDebugCallback();
			bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
			bool CheckExtensionSupport(const std::vector<const char*>& extensions);
            
#ifdef USE_VMA_ALLOCATOR
            void DebugDrawVmaMemory(VmaStatInfo& info, bool indent = true);
#endif

			std::vector<const char*> GetRequiredExtensions();
			std::vector<const char*> GetRequiredLayers();

		private:

			vk::Instance m_VkInstance;
			vk::DebugReportCallbackEXT m_DebugCallback;

			std::vector<vk::LayerProperties> m_InstanceLayers;
			std::vector<vk::ExtensionProperties> m_InstanceExtensions;

			std::vector<const char*> m_InstanceLayerNames;
			std::vector<const char*> m_InstanceExtensionNames;

			VKCommandPool* m_CommandPool;
			void* m_WindowContext;

			bool m_StandardValidationLayer = false;
			bool m_RenderDocLayer = false;
			bool m_AssistanceLayer = false;
		};
	}
}
