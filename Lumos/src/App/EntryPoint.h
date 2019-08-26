#if defined(LUMOS_PLATFORM_WINDOWS)

#include "Core/CoreSystem.h"
#include "Core/OS.h"

extern Lumos::Application* Lumos::CreateApplication();

int main(int argc, char** argv)
{
	Lumos::Internal::CoreSystem::Init();

	Lumos::CreateApplication();

	Lumos::OS::Create();
	Lumos::OS::Instance()->Run();
	Lumos::OS::Release();

	Lumos::Internal::CoreSystem::Shutdown();
}

#elif defined(LUMOS_PLATFORM_LINUX)

#include "Core/CoreSystem.h"

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

struct testStruct
{
    int id;
    String name;
    float power;
};

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


#endif
