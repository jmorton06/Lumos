#include "MacOSOS.h"
#include "MacOSPower.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "Core/Application.h"
#include "Core/OS/MemoryManager.h"

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

        LUMOS_LOG_INFO("--------------------");
        LUMOS_LOG_INFO(" System Information ");
        LUMOS_LOG_INFO("--------------------");

        if(state != PowerState::POWERSTATE_NO_BATTERY)
            LUMOS_LOG_INFO("Battery Info - Percentage : {0} , Time Left {1}s , State : {2}", percentage, secondsLeft, PowerStateToString(state));
        else
            LUMOS_LOG_INFO("Power - Outlet");

        auto systemInfo = MemoryManager::Get()->GetSystemInfo();
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

    void MacOSOS::SetTitleBarColour(const glm::vec4& colour, bool dark)
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

        [nativeWindow toggleFullScreen:nil];
    }
}
