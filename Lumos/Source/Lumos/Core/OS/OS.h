#pragma once
#include "Core/Core.h"
#include "Maths/MathsFwd.h"
#include "Utilities/TSingleton.h"
#include <string>

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

    struct SafeAreaInsets
    {
        float top = 0, bottom = 0, left = 0, right = 0;
    };

    class LUMOS_EXPORT OS : public TSingletonAbstract<OS>
    {
        friend class TSingletonAbstract<OS>;

    public:
        OS()          = default;
        virtual ~OS() = default;

        virtual void Run() = 0;
        static void Create();

        virtual SafeAreaInsets GetSafeAreaInsets() const { return {}; }

        static std::string PowerStateToString(PowerState state);

        virtual std::string GetExecutablePath() = 0;
        virtual std::string GetCurrentWorkingDirectory();
        virtual std::string GetAssetPath();
        virtual void Vibrate() const { };
        virtual void SetTitleBarColour(const Vec4& colour, bool dark = true) { };
        virtual void SetWindowDecorations(bool decorated) { };
        virtual void BeginWindowDrag() { };
        virtual void UpdateWindowDrag() { };
        // Resize edge bitmask: Top=1 Bottom=2 Left=4 Right=8 (corners = OR of two).
        virtual void BeginWindowResize(int edgeMask) { };
        virtual void UpdateWindowResize() { };
        virtual bool IsWindowMaximised() const { return false; }
        virtual bool IsWindowFullscreen() const { return false; }
        virtual void SetWindowFullscreen(bool fullscreen) { };
        virtual bool DidEnterFullscreen() const { return false; }
        virtual void RestoreWindow() { };
        virtual void IconifyWindow() { };

        // Mobile only
        virtual void ShowKeyboard(bool bShow) { };
        virtual void HideKeyboard() { };
        virtual void Delay(uint32_t usec) { };

        // Needed for MaxOS
        virtual void MaximiseWindow() { }

        virtual void OpenFileLocation(const std::string& path) { }
        virtual void OpenFileExternal(const std::string& path) { }
        virtual void OpenURL(const std::string& url) { }
        static void ConsoleWrite(const char* msg, u8 level);
    };
}
