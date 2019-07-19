#pragma once

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_LINUX) || defined(LUMOS_PLATFORM_WINDOWS)

#include "System/System.h"

extern Lumos::Application* Lumos::CreateApplication();

int main(int argc, char** argv)
{
	Lumos::Internal::CoreSystem::Init();

	auto app = Lumos::CreateApplication();
    app->Init();
	app->Run();
	delete app;

	Lumos::Internal::CoreSystem::Shutdown();
}

#elif defined(LUMOS_PLATFORM_IOS)

#include "Application.h"
#include "System/System.h"
#include "System/VFS.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

extern Lumos::Application* Lumos::CreateApplication();

namespace iosApp
{
	static Lumos::Application* app = nullptr;

    const std::string getAssetPath();

	static void Init()
	{
		Lumos::Internal::CoreSystem::Init();

		app = Lumos::CreateApplication();
		app->Init();
        
        String root = getAssetPath();
        Lumos::VFS::Get()->Mount("CoreShaders", root + "/Lumos/res/shaders");
        Lumos::VFS::Get()->Mount("CoreMeshes", root + "/Lumos/res/meshes");
        Lumos::VFS::Get()->Mount("CoreTextures", root + "/Lumos/res/textures");
	}

	static void OnKeyPressed()
	{
	}
    
	#ifdef LUMOS_RENDER_API_VULKAN

	static void SetIOSView(void* view)
	{
        Lumos::Graphics::VKDevice::m_IOSView = view;
	}
	#endif

	static void OnFrame()
	{
		app->OnFrame();
	}

	static void Quit()
	{
		delete app;

        Lumos::Internal::CoreSystem::Shutdown();
	}
}

#endif
