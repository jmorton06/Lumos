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

#include <sys/sysctl.h>
#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioServices.h>

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
        
        Lumos::Debug::Log::Info("Device : {0}",GetModelName());
        Init();
        
        s_Instance = this;

        auto app = Lumos::CreateApplication();
        app->Init();
    }

    iOSOS::~iOSOS()
    {
        Lumos::Application::Release();
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
    
    String iOSOS::GetAssetPath()
    {
        return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/"].UTF8String;
    }
    
    void iOSOS::OnKeyPressed(char keycode, bool down)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnKeyEvent((Lumos::InputCode::Key)Lumos::iOSKeyCodes::iOSKeyToLumos(keycode), down);
    }

    void iOSOS::OnScreenPressed(u32 x, u32 y, u32 count, bool down)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnTouchEvent(x,y,count, down);
    }

    void iOSOS::OnMouseMovedEvent(u32 xPos, u32 yPos)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnMouseMovedEvent(xPos,yPos);
    }
    
    void iOSOS::OnScreenResize(u32 width, u32 height)
    {
        ((iOSWindow*)Application::Instance()->GetWindow())->OnResizeEvent(width, height);
    }

    String iOSOS::GetExecutablePath()
    {
        return GetAssetPath();
        static char path[512] = "";
        if (!path[0]) 
        {
            NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
            strcpy(path, (const char *)[bundlePath cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        return path;
    }

    void iOSOS::Vibrate() const
    {
        @autoreleasepool
        {
            AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
        }
    }

    void iOSOS::Alert(const char *message, const char *title)
    {
        NSString* nsmsg = [[NSString alloc] initWithBytes:message
                            length:strlen(message) * sizeof(message)
                            encoding:NSUTF8StringEncoding];
        
        UIAlertController* alert = [UIAlertController
                                      alertControllerWithTitle:@"Error"
                                      message:nsmsg
                                      preferredStyle:UIAlertControllerStyleAlert];
          
        UIAlertAction* okButton = [UIAlertAction
                                     actionWithTitle:@"OK"
                                     style:UIAlertActionStyleDefault
                                     handler:^(UIAlertAction* action) {
                                          // Handle your ok button action here
                                     }];
          
        [alert addAction:okButton];
        
        UIViewController* viewController = [[[[UIApplication sharedApplication] delegate] window] rootViewController];
        [viewController presentViewController:alert animated:YES completion:nil];
    }
    
    String iOSOS::GetModelName() const
    {
        size_t size;
        sysctlbyname("hw.machine", NULL, &size, NULL, 0);
        char *model = (char *)malloc(size);
        if (model == NULL) {
            return "";
        }
        sysctlbyname("hw.machine", model, &size, NULL, 0);
        NSString *platform = [NSString stringWithCString:model encoding:NSUTF8StringEncoding];
        free(model);
        const char *str = [platform UTF8String];
        return String(str != NULL ? str : "");
    }
}
