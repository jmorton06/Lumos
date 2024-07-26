#include "AndroidOS.h"
#include "Core/VFS.h"
#include "Core/OS/Input.h"
#include "Core/CoreSystem.h"
#include "Core/Application.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

#include <sys/sysctl.h>
#include <SystemConfiguration/SCNetworkReachability.h>
#include <netinet/in.h>

#define MAX_SIMULTANEOUS_TOUCHES 10

namespace Lumos
{
    static AndroidOS* os = nullptr;

    AndroidOS::AndroidOS()
    {
    }

    AndroidOS::~AndroidOS()
    {
    }

    void AndroidOS::Init()
    {
        Lumos::Internal::CoreSystem::Init(false);

        std::string root = GetAssetPath();
        Lumos::VFS::Get().Mount("CoreShaders", root + "Shaders");

        // Lumos::VFS::Get().Mount("Shaders", root + "shaders");
        Lumos::VFS::Get().Mount("Meshes", root + "Meshes");
        Lumos::VFS::Get().Mount("Textures", root + "Textures");
        Lumos::VFS::Get().Mount("Scripts", root + "Scripts");
        Lumos::VFS::Get().Mount("Scenes", root + "Scenes");
        Lumos::VFS::Get().Mount("Sounds", root + "Sounds");
        Lumos::VFS::Get().Mount("Assets", root + "Assets");

        LINFO("Device : %s", GetModelName());

        // AndroidWindow::MakeDefault();

        auto app = Lumos::CreateApplication();
        app->Init();
    }

    void AndroidOS::OnFrame()
    {
        Application::Get().OnFrame();
    }

    void AndroidOS::OnQuit()
    {
        Application::Get().OnQuit();
        Application::Release();
        Lumos::Internal::CoreSystem::Shutdown();
    }

    std::string AndroidOS::GetAssetPath()
    {
        return "";
    }

    void AndroidOS::OnKeyPressed(char keycode, bool down)
    {
    }

    void AndroidOS::OnScreenPressed(uint32_t x, uint32_t y, uint32_t count, bool down)
    {
        //((AndroidOS*)Application::Get().GetWindow())->OnTouchEvent(x,y,count, down);
    }

    void AndroidOS::OnMouseMovedEvent(uint32_t xPos, uint32_t yPos)
    {
        //((AndroidWindow*)Application::Get().GetWindow())->OnMouseMovedEvent(xPos,yPos);
    }

    void AndroidOS::OnScreenResize(uint32_t width, uint32_t height)
    {
        //((AndroidWindow*)Application::Get().GetWindow())->OnResizeEvent(width, height);
    }

    std::string AndroidOS::GetExecutablePath()
    {
        return "";
    }

    void AndroidOS::Vibrate() const
    {
    }

    void AndroidOS::Alert(const char* message, const char* title)
    {
    }

    std::string AndroidOS::GetModelName() const
    {
        return "Android device";
    }

    void AndroidOS::ShowKeyboard(bool open)
    {
    }

    bool AndroidOS::HasWifiConnection()
    {
        return true;
    }
}
