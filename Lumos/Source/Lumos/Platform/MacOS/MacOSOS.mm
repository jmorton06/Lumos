#include "MacOSOS.h"
#include "MacOSPower.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "Core/Application.h"
#include "Core/OS/MemoryManager.h"
#include "Maths/Vector4.h"

#include <mach-o/dyld.h>

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

        LINFO("--------------------");
        LINFO(" System Information ");
        LINFO("--------------------");

        if(state != PowerState::POWERSTATE_NO_BATTERY)
			LINFO("Battery Info - %i%%, Time Left: %ih %im, State: %s", percentage, hours, minutes, PowerStateToString(state).c_str());
        else
            LINFO("Power - Outlet");

        auto systemInfo = MemoryManager::Get().GetSystemInfo();
        systemInfo.Log();

        auto& app = Lumos::Application::Get();

        app.Init();
        app.Run();
        app.Release();
    }

    void MacOSOS::Init()
    {
        GLFWWindow::MakeDefault();
    }

    void MacOSOS::SetTitleBarColour(const Vec4& colour, bool dark)
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

    void MacOSOS::SetWindowDecorations(bool decorated)
    {
        auto& app = Lumos::Application::Get();
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));

        if(!decorated)
        {
            window.styleMask |= NSWindowStyleMaskFullSizeContentView;
            window.titlebarAppearsTransparent = YES;
            window.titleVisibility = NSWindowTitleHidden;

            SEL sel = NSSelectorFromString(@"_setTitlebarHeight:");
            if([window respondsToSelector:sel])
            {
                NSMethodSignature* sig = [NSWindow instanceMethodSignatureForSelector:sel];
                if(sig)
                {
                    NSInvocation* inv = [NSInvocation invocationWithMethodSignature:sig];
                    [inv setTarget:window];
                    [inv setSelector:sel];
                    CGFloat barH = 42.0;
                    [inv setArgument:&barH atIndex:2];
                    [inv invoke];
                }
            }
        }
        else
        {
            window.styleMask &= ~NSWindowStyleMaskFullSizeContentView;
            window.titlebarAppearsTransparent = NO;
            window.titleVisibility = NSWindowTitleVisible;
        }

        app.GetWindow()->RefreshSize();
    }

    std::string MacOSOS::GetExecutablePath()
    {
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);

        TDArray<char> buffer;
        buffer.Resize(size + 1);

        _NSGetExecutablePath(buffer.Data(), &size);
        buffer[size] = '\0';

        if (!strrchr(buffer.Data(), '/'))
        {
            return "";
        }
        return std::string(buffer.Data());
    }

	void MacOSOS::Delay(uint32_t usec)
	{
		struct timespec requested = { static_cast<time_t>(usec / 1000000), (static_cast<long>(usec) % 1000000) * 1000 };
		struct timespec remaining;
		while (nanosleep(&requested, &remaining) == -1)
		{
			requested.tv_sec = remaining.tv_sec;
			requested.tv_nsec = remaining.tv_nsec;
		}
	}

    void MacOSOS::MaximiseWindow()
    {
        auto window = Application::Get().GetWindow();
        NSWindow* nativeWindow = glfwGetCocoaWindow((GLFWwindow*)window->GetHandle());

        [nativeWindow zoom:nil];
    }

    // Latched drag state — captured at BeginWindowDrag.
    static NSPoint s_DragStartMouseScreen = { 0, 0 };
    static NSPoint s_DragStartWindowOrigin = { 0, 0 };

    bool MacOSOS::BeginWindowDrag()
    {
        auto& app = Lumos::Application::Get();
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));
        s_DragStartMouseScreen   = [NSEvent mouseLocation];
        s_DragStartWindowOrigin  = window.frame.origin;
        return false;
    }

    void MacOSOS::UpdateWindowDrag()
    {
        auto& app = Lumos::Application::Get();
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));
        NSPoint nowScreen = [NSEvent mouseLocation];
        NSPoint target;
        target.x = s_DragStartWindowOrigin.x + (nowScreen.x - s_DragStartMouseScreen.x);
        target.y = s_DragStartWindowOrigin.y + (nowScreen.y - s_DragStartMouseScreen.y);
        [window setFrameOrigin:target];
    }

    bool MacOSOS::IsWindowMaximised() const
    {
        auto& app = Lumos::Application::Get();
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));
        return [window isZoomed];
    }

    bool MacOSOS::IsWindowFullscreen() const
    {
        auto& app = Lumos::Application::Get();
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));
        return ([window styleMask] & NSWindowStyleMaskFullScreen) != 0;
    }

    static bool s_DidEnterFullscreen = false;

    void MacOSOS::SetWindowFullscreen(bool fullscreen)
    {
        auto& app = Lumos::Application::Get();
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));

        window.collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
        window.styleMask |= NSWindowStyleMaskResizable;

        static bool observerRegistered = false;
        if(!observerRegistered)
        {
            observerRegistered = true;
            [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidEnterFullScreenNotification
                                                              object:nil
                                                               queue:[NSOperationQueue mainQueue]
                                                          usingBlock:^(NSNotification*) { s_DidEnterFullscreen = true; }];
        }

        bool isFullscreen = ([window styleMask] & NSWindowStyleMaskFullScreen) != 0;
        if(isFullscreen != fullscreen)
        {
            if(fullscreen)
            {
                [NSApp activateIgnoringOtherApps:YES];
                [window makeKeyAndOrderFront:nil];
            }
            [window toggleFullScreen:nil];
        }
    }

    bool MacOSOS::DidEnterFullscreen() const
    {
        return s_DidEnterFullscreen;
    }

    void MacOSOS::RestoreWindow()
    {
        auto& app = Lumos::Application::Get();
        NSWindow* window = (NSWindow*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));
        if([window isZoomed])
            [window zoom:nil];
    }
}
