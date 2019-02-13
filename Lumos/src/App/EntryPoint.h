#pragma once

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_LINUX) || defined(LUMOS_PLATFORM_WINDOWS)

#include "System/System.h"

extern Lumos::Application* Lumos::CreateApplication();

int main(int argc, char** argv)
{
	Lumos::internal::System::Init();

	auto app = Lumos::CreateApplication();
    app->Init();
	app->Run();
	delete app;

	Lumos::internal::System::Shutdown();
}

#elif defined(LUMOS_PLATFORM_IOS)

#include "Application.h"
#include "System/System.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

extern Lumos::Application* Lumos::CreateApplication();

namespace iosApp
{
	static Lumos::Application* app = nullptr;

	static void Init()
	{
		Lumos::internal::System::Init();

		app = Lumos::CreateApplication();
		app->Init();
	}

	static void OnKeyPressed()
	{
	}

	#ifdef LUMOS_RENDER_API_VULKAN

	static void SetIOSView(void* view)
	{
        Lumos::graphics::VKDevice::m_IOSView = view;
	}
	#endif

	static void OnFrame()
	{
		app->OnFrame();
	}

	static void Quit()
	{
		delete app;

		Lumos::internal::System::Shutdown();
	}
}

#endif
