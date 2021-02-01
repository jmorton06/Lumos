#include "MacOSOS.h"
#include "MacOSPower.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "Core/Application.h"

#include <mach-o/dyld.h>

#import <Cocoa/Cocoa.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void MacOSOS::Run()
    {
        auto power = MacOSPower();
        auto percentage = power.GetPowerPercentageLeft();
        auto secondsLeft = power.GetPowerSecondsLeft();
        auto state = power.GetPowerState();
		
		int hours, minutes;
		minutes = secondsLeft / 60;
		hours = minutes / 60;
		minutes = minutes % 60;
		
        LUMOS_LOG_INFO("Battery Info - {0}% , Time Left {1}h : {2}m , State : {3}", percentage, hours, minutes, PowerStateToString(state));
        
        const std::string root = ROOT_DIR;
		VFS::Get()->Mount("Meshes", root + "/Assets/meshes");
		VFS::Get()->Mount("Textures", root + "/Assets/textures");
		VFS::Get()->Mount("Sounds", root + "/Assets/sounds");
        
        auto& app = Lumos::Application::Get();

        app.Init();
        app.Run();
        app.Release();
    }

    void MacOSOS::Init()
    {
        GLFWWindow::MakeDefault();
    }

    void MacOSOS::SetTitleBarColour(const Maths::Vector4& colour, bool dark)
    {
        auto& app = Lumos::Application::Get();

        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));
        window.titlebarAppearsTransparent = YES;
        //window.titleVisibility = NSWindowTitleHidden;
        
        NSColor *titleColour = [NSColor colorWithSRGBRed:colour.x green:colour.y blue:colour.z alpha:colour.w];
        window.backgroundColor = titleColour;
        if(dark)
            window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];
        else
            window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantLight];
    }

    std::string MacOSOS::GetExecutablePath()
    {
        std::string result;

        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);

        std::vector<char> buffer;
        buffer.resize(size + 1);

        _NSGetExecutablePath(buffer.data(), &size);
        buffer[size] = '\0';

        if (!strrchr(buffer.data(), '/'))
        {
            return "";
        }
        return buffer.data();
    }
}
