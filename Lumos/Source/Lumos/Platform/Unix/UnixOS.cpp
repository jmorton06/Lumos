#include "Precompiled.h"
#include "UnixOS.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "Core/OS/MemoryManager.h"
#include "Core/Application.h"

#include <time.h>
#ifdef LUMOS_PLATFORM_MACOS
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void UnixOS::Run()
    {
        auto& app = Lumos::Application::Get();

        LINFO("--------------------");
        LINFO(" System Information ");
        LINFO("--------------------");

        auto systemInfo = MemoryManager::Get()->GetSystemInfo();
        systemInfo.Log();

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

#ifdef LUMOS_PLATFORM_MACOS
        int64_t total_physical;
        size_t len = sizeof(total_physical);
        sysctlbyname("hw.memsize", &total_physical, &len, NULL, 0);

        mach_port_t mach_port = mach_host_self();
        vm_size_t page_size;
        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        vm_statistics64_data_t vm_stats;

        if(host_page_size(mach_port, &page_size) != KERN_SUCCESS)
        {
            perror("Failed to get page size");
            exit(EXIT_FAILURE);
        }

        if(host_statistics64(mach_port, HOST_VM_INFO, (host_info64_t)&vm_stats, &count) != KERN_SUCCESS)
        {
            perror("Failed to get VM statistics");
            exit(EXIT_FAILURE);
        }

        int64_t available_physical = (int64_t)vm_stats.free_count * (int64_t)page_size;
        int64_t total_virtual      = (int64_t)(vm_stats.wire_count + vm_stats.active_count + vm_stats.inactive_count + vm_stats.free_count) * (int64_t)page_size;
        int64_t available_virtual  = (int64_t)vm_stats.free_count * (int64_t)page_size;

        SystemMemoryInfo result = {
            available_physical,
            total_physical,
            available_virtual,
            total_virtual
        };

        return result;
#else
        SystemMemoryInfo result = {};
        return result;
#endif
    }

    void UnixOS::Delay(uint32_t usec)
    {
        struct timespec requested = { static_cast<time_t>(usec / 1000000), (static_cast<long>(usec) % 1000000) * 1000 };
        struct timespec remaining;
        while(nanosleep(&requested, &remaining) == -1)
        {
            requested.tv_sec  = remaining.tv_sec;
            requested.tv_nsec = remaining.tv_nsec;
        }
    }

    void UnixOS::OpenFileLocation(const std::string& path)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open -R " + path;
        std::system(command.c_str());
#endif
    }

    void UnixOS::OpenFileExternal(const std::string& path)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open " + path;
        std::system(command.c_str());
#endif
    }

    void UnixOS::OpenURL(const std::string& url)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open " + url;
        system(command.c_str());
#endif
    }
}
