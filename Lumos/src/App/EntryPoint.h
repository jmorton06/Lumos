#pragma once

#if defined(LUMOS_PLATFORM_LINUX) || defined(LUMOS_PLATFORM_WINDOWS)

#include "System/CoreSystem.h"

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

#elif defined(LUMOS_PLATFORM_MACOS)

#include "Core/OS.h"

int main(int argc, char** argv)
{
    Lumos::Internal::CoreSystem::Init();
    
    Lumos::CreateApplication();
    
    Lumos::OS::Create();
    Lumos::OS::Instance()->Run();
    Lumos::OS::Release();
    
    Lumos::Internal::CoreSystem::Shutdown();
}

#elif defined(LUMOS_PLATFORM_IOS)

#include "Application.h"
#include "System/CoreSystem.h"
#include "System/VFS.h"

#include "Platform/iOS/iOSOS.h"

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
        static_cast<Lumos::iOSOS*>(Lumos::OS::Instance())->SetIOSView(view);
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
