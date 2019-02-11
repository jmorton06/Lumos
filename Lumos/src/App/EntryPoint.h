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
extern Lumos::Application* Lumos::CreateApplication();

namespace iosApp
{
	Lumos::Application* app;

	void Init()
	{
		Lumos::internal::System::Init();

		app = Lumos::CreateApplication();
		app->Init();
	}

	void OnKeyPressed()
	{
	}

	void OnFrame()
	{
		app->OnFrame();
	}

	void Quit()
	{
		delete app;

		Lumos::internal::System::Shutdown();
	}
}

#endif
