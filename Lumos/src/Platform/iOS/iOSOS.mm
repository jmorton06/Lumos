#include "iOSOS.h"
#include "iOSWindow.h"
#include "iOSKeyCodes.h"
#include "Application.h"
#include "Core/VFS.h"
#include "Core/OS/Input.h"
#include "Core/CoreSystem.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKDevice.h"
#endif

#import <UIKit/UIKit.h>

namespace Lumos
{
    float iOSOS::m_X = 0.0f;
    float iOSOS::m_Y = 0.0f;

    iOSOS::iOSOS()
    {
        Lumos::Internal::CoreSystem::Init(false);

        String root = GetAssetPath();
        Lumos::VFS::Get()->Mount("CoreShaders", root + "shaders");
        Lumos::VFS::Get()->Mount("CoreMeshes", root + "meshes");
        Lumos::VFS::Get()->Mount("CoreTextures", root + "textures");

        Lumos::VFS::Get()->Mount("Shaders", root + "shaders");
        Lumos::VFS::Get()->Mount("Meshes", root + "meshes");
        Lumos::VFS::Get()->Mount("Textures", root + "textures");
        
        Init();
        
        s_Instance = this;

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
    
    void iOSOS::OnKeyPressed(char keycode)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnKeyEvent((Lumos::InputCode::Key)Lumos::iOSKeyCodes::iOSKeyToLumos(keycode), true);
    }

    void iOSOS::OnScreenPressed(u32 x, u32 y, u32 count)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnTouchEvent(x,y,count, true);
    }

    const char* iOSOS::GetExecutablePath()
    {
        return GetAssetPath().c_str();
        static char path[512] = "";
        if (!path[0]) 
        {
            NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
            strcpy(path, (const char *)[bundlePath cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        return path;
}
}
