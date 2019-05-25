#pragma once

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_LINUX) || defined(LUMOS_PLATFORM_WINDOWS)

#include "System/System.h"

extern lumos::Application* lumos::CreateApplication();

int main(int argc, char** argv)
{
	lumos::internal::System::Init();

	auto app = lumos::CreateApplication();
    app->Init();
	app->Run();
	delete app;

	lumos::internal::System::Shutdown();
}

#elif defined(LUMOS_PLATFORM_IOS)

#include "Application.h"
#include "System/System.h"
#include "System/VFS.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

extern lumos::Application* lumos::CreateApplication();

namespace iosApp
{
	static lumos::Application* app = nullptr;

    const std::string getAssetPath();

	static void Init()
	{
		lumos::internal::System::Init();

		app = lumos::CreateApplication();
		app->Init();
        
        String root = getAssetPath();
        lumos::VFS::Get()->Mount("CoreShaders", root + "/lumos/res/shaders");
        lumos::VFS::Get()->Mount("CoreMeshes", root + "/lumos/res/meshes");
        lumos::VFS::Get()->Mount("CoreTextures", root + "/lumos/res/textures");
	}

	static void OnKeyPressed()
	{
	}
    
	#ifdef LUMOS_RENDER_API_VULKAN

	static void SetIOSView(void* view)
	{
        lumos::graphics::VKDevice::m_IOSView = view;
	}
	#endif

	static void OnFrame()
	{
		app->OnFrame();
	}

	static void Quit()
	{
		delete app;

		lumos::internal::System::Shutdown();
	}
}

#endif
