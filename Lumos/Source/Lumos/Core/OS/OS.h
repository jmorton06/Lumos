#pragma once
#include "Core/Core.h"
#include "Maths/MathsFwd.h"

namespace Lumos
{
    enum PowerState
    {
        POWERSTATE_UNKNOWN,
        POWERSTATE_ON_BATTERY,
        POWERSTATE_NO_BATTERY,
        POWERSTATE_CHARGING,
        POWERSTATE_CHARGED
    };

    class LUMOS_EXPORT OS
    {
    public:
        OS()          = default;
        virtual ~OS() = default;

        virtual void Run() = 0;

        static void Create();
        static void Release();
        static void SetInstance(OS* instance)
        {
            s_Instance = instance;
        }

        static OS* Instance()
        {
            return s_Instance;
        }
        static std::string PowerStateToString(PowerState state);

        virtual std::string GetExecutablePath() = 0;
        virtual std::string GetCurrentWorkingDirectory();
        virtual std::string GetAssetPath();
        virtual void Vibrate() const {};
        virtual void SetTitleBarColour(const Vec4& colour, bool dark = true) {};

        // Mobile only
        virtual void ShowKeyboard() {};
        virtual void HideKeyboard() {};
        virtual void Delay(uint32_t usec) {};

        // Needed for MaxOS
        virtual void MaximiseWindow() { }

        virtual void OpenFileLocation(const std::string& path) { }
        virtual void OpenFileExternal(const std::string& path) { }
        virtual void OpenURL(const std::string& url) { }
        static void ConsoleWrite(const char* msg, u8 level);

    protected:
        static OS* s_Instance;
    };
}
