#pragma once

#if defined(JM_PLATFORM_MACOS) || defined(JM_PLATFORM_LINUX) || defined(JM_PLATFORM_WINDOWS)

#include "System/System.h"

extern jm::Application* jm::CreateApplication();

int main(int argc, char** argv)
{
	jm::internal::System::Init();

	auto app = jm::CreateApplication();
    app->Init();
	app->Run();
	delete app;

	jm::internal::System::Shutdown();
}
#endif
