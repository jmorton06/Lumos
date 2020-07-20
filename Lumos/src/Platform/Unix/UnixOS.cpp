#include "lmpch.h"
#include "UnixOS.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "Core/OS/MemoryManager.h"
#include "Core/Application.h"

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
	void UnixOS::Run()
	{
		const std::string root = ROOT_DIR;
		VFS::Get()->Mount("Meshes", root + "/Assets/meshes");
		VFS::Get()->Mount("Textures", root + "/Assets/textures");
		VFS::Get()->Mount("Sounds", root + "/Assets/sounds");

		auto& app = Lumos::Application::Get();
		app.Init();
		app.Run();
		app.Release();
	}

	void UnixOS::Init()
	{
		GLFWWindow::MakeDefault();
	}

	SystemMemoryInfo MemoryManager::GetSystemInfo()
	{
		return SystemMemoryInfo();
	}
}
