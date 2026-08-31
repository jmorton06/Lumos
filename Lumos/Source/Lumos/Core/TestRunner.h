#pragma once

#include "Utilities/TSingleton.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/Vector2.h"
#include <string>

namespace sol
{
    class state;
}

namespace Lumos
{
    // Drives a Lua script as a coroutine, one resume per frame, injecting input
    // straight into Input so the game and UI are exercised the way a player would.
    // Enabled with --test=<path.lua>; sets the process exit code on failure.
    class LUMOS_EXPORT TestRunner : public TSingleton<TestRunner>
    {
        friend class TSingleton<TestRunner>;

    public:
        TestRunner();
        ~TestRunner();

        // scriptPath is a physical path, not VFS - a stale asset pack must not shadow tests.
        void Init(const std::string& scriptPath);
        bool Active() const { return m_Active; }

        void BindLua(sol::state& state);

        // After Window::ProcessInput, before UIBeginFrame: reassert injected state, then resume.
        void Step();
        // After Present, when the swap chain image is readable. canCapture is false
        // when there is nothing presentable this frame (minimised, no renderer).
        void ServiceScreenshots(bool canCapture);

        void Log(const std::string& msg);
        void Fail(const std::string& msg);
        void Finish(bool passed);

        void SetUpdateGolden(bool v) { m_UpdateGolden = v; }
        void SetTimeoutFrames(int frames) { m_TimeoutFrames = frames; }
        void SetReportPath(const std::string& path) { m_ReportPath = path; }

        // Called from the Lua bindings.
        void SetMousePos(float x, float y);
        void SetMousePosRaw(const Vec2& raw);
        // Parks the cursor on a live widget found by its displayed text.
        bool MoveToWidget(const std::string& text, bool substring = false);
        void SetMouseButton(int button, bool down);
        void SetKey(int key, bool down);
        void SetScroll(float y, float x);
        // compare=false captures for human review without gating the run - use it
        // for frames whose world state can't be pinned (long sims, live traffic).
        void RequestScreenshot(const std::string& name, bool compare);

    private:
        void Resume();
        void ReapplyStickyInput();
        void WriteReport();
        bool CompareAgainstGolden(const std::string& name, const std::string& actualPath);

        struct Impl;
        Impl* m_Impl = nullptr;

        bool m_Active       = false;
        bool m_Finished     = false;
        bool m_Failed       = false;
        int m_Frame         = 0;
        int m_TimeoutFrames = 3000;

        bool m_UpdateGolden = false;
        std::string m_ScriptPath;
        std::string m_ReportPath;
        std::string m_GoldenDir;
        std::string m_OutDir;

        bool m_HasMousePos = false;
        Vec2 m_MousePos    = Vec2(0.0f);
        bool m_MouseHeld[32]  = {};
        bool m_KeyHeld[1024]  = {};

        struct PendingShot
        {
            std::string name;
            bool compare = true;
        };

        TDArray<std::string> m_Failures;
        TDArray<PendingShot> m_PendingShots;
        int m_Checks = 0;
    };
}
