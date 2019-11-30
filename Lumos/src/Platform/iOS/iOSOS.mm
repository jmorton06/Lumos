#include "iOSOS.h"
#include "Application.h"
#include "Core/VFS.h"

#include "Platform/Unix/UnixThread.h"
#include "Platform/Unix/UnixMutex.h"
#include "Platform/GLFM/GLFMWindow.h"

#include "Core/CoreSystem.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

#import <UIKit/UIKit.h>

namespace Lumos
{
    iOSOS::iOSOS()
    {
        Lumos::Internal::CoreSystem::Init(false);
        Init();

        auto app = Lumos::CreateApplication();
        app->Init();

        String root = GetAssetPath();
        Lumos::VFS::Get()->Mount("CoreShaders", root + "/Lumos/res/shaders");
        Lumos::VFS::Get()->Mount("CoreMeshes", root + "/Lumos/res/meshes");
        Lumos::VFS::Get()->Mount("CoreTextures", root + "/Lumos/res/textures");
    }

    iOSOS::~iOSOS()
    {

    }

    void iOSOS::Init()
    {
        UnixThread::MakeDefault();
        UnixMutex::MakeDefault();
        GLFMWindow::MakeDefault();
    }

    void iOSOS::OnFrame()
    {
        Application::Instance()->OnFrame();
    }
    
    void iOSOS::OnQuit()
    {
        delete Application::Instance();
	    Lumos::Internal::CoreSystem::Shutdown();
    }
    
    String iOSOS::GetAssetPath() const
    {
        return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/"].UTF8String;
    }
    
    void iOSOS::OnKeyPressed(u32 keycode)
    {
        
    }
}
