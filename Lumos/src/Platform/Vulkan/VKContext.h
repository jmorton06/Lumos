#pragma once
#include "Graphics/API/GraphicsContext.h"

#include "VK.h"

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

		const std::vector<const char*> validationLayers =
		{
			"VK_LAYER_LUNARG_standard_validation"
		};

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		class VKContext : public GraphicsContext
		{
		public:
			VKContext(const WindowProperties& properties, void* deviceContext);
			~VKContext();

			void Init() override;
			void Present() override;
			void Unload();

			inline static VKContext* Get() { return static_cast<VKContext*>(s_Context); }

			static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugReportFlagsEXT flags, vk::DebugReportObjectTypeEXT objType,
			                                                    uint64_t obj, size_t location, int32_t code,
			                                                    const char* layerPrefix, const char* msg, void* userData);

			vk::Instance GetVKInstance() const { return m_VkInstance; }

			void AddInstanceLayer(const char* instanceLayerName)
			{
				m_InstanceLayers.push_back(instanceLayerName);
			}

			void AddInstanceExtension(const char* instanceExtensionName)
			{
				m_InstanceExtensions.push_back(instanceExtensionName);
			}

			VKCommandPool* GetCommandPool() const { return m_CommandPool; }

			size_t GetMinUniformBufferOffsetAlignment() const override;

			void* GetWindowContext() const { return m_WindowContext; }
            
            bool FlipImGUITexture() const override { return false; }

		protected:

			void CreateInstance();
			void SetupDebugCallback();

		private:

			vk::Instance m_VkInstance{};
			vk::DebugReportCallbackEXT callback{};

			std::vector<const char*> m_InstanceLayers;
			std::vector<const char*> m_InstanceExtensions;

			VKCommandPool* m_CommandPool;
			void* m_WindowContext;
		};
	}
}
