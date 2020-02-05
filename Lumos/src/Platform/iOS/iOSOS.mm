#include "iOSOS.h"
#include "iOSWindow.h"
#include "Application.h"
#include "Core/VFS.h"
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

        String root = GetAssetPath();
        Lumos::VFS::Get()->Mount("CoreShaders", root + "Lumos/res/shaders");
        Lumos::VFS::Get()->Mount("CoreMeshes", root + "Lumos/res/meshes");
        Lumos::VFS::Get()->Mount("CoreTextures", root + "Lumos/res/textures");
        
        Init();

        auto app = Lumos::CreateApplication();
        app->Init();
    }

    iOSOS::~iOSOS()
    {

    }

    void iOSOS::Init()
    {
        iOSWindow::MakeDefault();
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

    const char* iOSOS::GetExecutablePath()
    {
        static char path[512] = "";
        if (!path[0]) 
        {
            NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
            strcpy(path, (const char *)[bundlePath cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        return path;
}
}
