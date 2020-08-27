#pragma once
#include "Graphics/API/GraphicsContext.h"

#include "VK.h"
#include "Core/Reference.h"

#ifdef USE_VMA_ALLOCATOR
#include <vulkan/vk_mem_alloc.h>
#endif

#ifdef LUMOS_DEBUG
const bool EnableValidationLayers = true;
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
            
			_FORCE_INLINE_ static VKContext* Get() { return static_cast<VKContext*>(s_Context); }
            
			static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags,
                                                                VkDebugReportObjectTypeEXT objType,
                                                                uint64_t sourceObj,
                                                                size_t location,
                                                                int32_t msgCode,
                                                                const char* pLayerPrefix,
                                                                const char* pMsg,
                                                                void* userData);
            
			VkInstance GetVKInstance()		const { return m_VkInstance; }
			void* GetWindowContext()			const { return m_WindowContext; }
            
			const Ref<VKCommandPool>& GetCommandPool() const { return m_CommandPool; }
            
			size_t GetMinUniformBufferOffsetAlignment() const override;
            
            bool FlipImGUITexture() const override { return false; }
            void WaitIdle() const override;
			void OnImGui() override;
            
			const std::vector<const char*>& GetLayerNames()			const { return m_InstanceLayerNames; }
			const std::vector<const char*>& GetExtensionNames()		const { return m_InstanceExtensionNames; }
            
            static void MakeDefault();
            protected:
            static GraphicsContext* CreateFuncVulkan(const WindowProperties&, void*);
            
			void CreateInstance();
			void SetupDebugCallback();
			bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
			bool CheckExtensionSupport(const std::vector<const char*>& extensions);
            
#ifdef USE_VMA_ALLOCATOR
            void DebugDrawVmaMemory(VmaStatInfo& info, bool indent = true);
#endif
            
			static const std::vector<const char*> GetRequiredExtensions();
			const std::vector<const char*> GetRequiredLayers() const;
            
            private:
            
			VkInstance m_VkInstance;
			VkDebugReportCallbackEXT m_DebugCallback{};
            
			std::vector<VkLayerProperties> m_InstanceLayers;
			std::vector<VkExtensionProperties> m_InstanceExtensions;
            
			std::vector<const char*> m_InstanceLayerNames;
			std::vector<const char*> m_InstanceExtensionNames;
            
			Ref<VKCommandPool> m_CommandPool;
			void* m_WindowContext;
            
			bool m_StandardValidationLayer = false;
			bool m_RenderDocLayer = false;
			bool m_AssistanceLayer = false;
		};
	}
}
