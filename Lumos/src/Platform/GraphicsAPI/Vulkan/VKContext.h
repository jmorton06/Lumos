#pragma once
#include "Graphics/API/Context.h"

#include "Dependencies/vulkan/vulkan.h"

#ifdef LUMOS_RELEASE
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = false;
#endif

namespace Lumos
{
	namespace graphics
	{
		class VKCommandPool;

		const std::vector<const char*> validationLayers =
		{
			"VK_LAYER_LUNARG_standard_validation"
		};

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		class VKContext : public Context
		{
		public:
			VKContext(WindowProperties properties, void* deviceContext);
			~VKContext();

			void Init() override;
			void Present() override;
			void Unload();

			inline static VKContext* Get() { return static_cast<VKContext*>(s_Context); }

			static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
			                                                    uint64_t obj, size_t location, int32_t code,
			                                                    const char* layerPrefix, const char* msg, void* userData);

			VkInstance GetVKInstance() const { return m_VkInstance; }

			void AddInstanceLayer(const char* instanceLayerName)
			{
				instanceLayers.push_back(instanceLayerName);
			}

			void AddInstanceExtension(const char* instanceExtensionName)
			{
				instanceExtensions.push_back(instanceExtensionName);
			}

			VKCommandPool* GetCommandPool() const { return m_CommandPool; }

			size_t GetMinUniformBufferOffsetAlignment() const override;

			void* GetWindowContext() const { return m_WindowContext; }

		protected:

			void CreateInstance();
			void SetupDebugCallback();

		private:

			VkInstance m_VkInstance{};
			VkDebugReportCallbackEXT callback{};

			std::vector<const char*> instanceLayers;
			std::vector<const char*> instanceExtensions;

			VKCommandPool* m_CommandPool;
			void* m_WindowContext;
		};
	}
}
