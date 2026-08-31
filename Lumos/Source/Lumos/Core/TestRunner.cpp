#include "Precompiled.h"
#include "TestRunner.h"

#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/SwapChain.h"
#include "Graphics/UI.h"
#include "Maths/MathsUtilities.h"
#include "Scripting/Lua/LuaManager.h"
#include "Utilities/ImageExport.h"

#include <sol/sol.hpp>
#include "stb_image.h"

#include <filesystem>
#include <fstream>

namespace Lumos
{
    // A pixel counts as different when any channel differs by more than this.
    static constexpr int CHANNEL_THRESHOLD = 8;
    // ...and the image fails when more than this fraction of pixels differ.
    static constexpr float FAIL_FRACTION = 0.002f;

    // Plain Lua coroutines rather than sol's wrappers - fewer moving parts, and
    // the script gets the main state's globals so it can reach every binding.
    static const char* kPrelude = R"LUA(
Test.__co = nil
Test.__ABORT = {} -- sentinel: a failed Assert unwinds instead of cascading

function Test.__start(path)
    local chunk, err = loadfile(path)
    if not chunk then return err end
    Test.__co = coroutine.create(chunk)
    return nil
end

function Test.__step()
    if not Test.__co then return "dead", nil end
    if coroutine.status(Test.__co) == "dead" then return "dead", nil end
    local ok, err = coroutine.resume(Test.__co)
    if not ok then
        if err == Test.__ABORT then return "aborted", nil end
        return "error", tostring(err)
    end
    return coroutine.status(Test.__co), nil
end

function Test.Wait(frames)
    for _ = 1, (frames or 1) do coroutine.yield() end
end

-- Returns false on timeout without recording anything; the caller's Assert
-- supplies the message that actually explains what was being waited for.
function Test.WaitUntil(fn, timeout)
    timeout = timeout or 600
    for _ = 1, timeout do
        if fn() then return true end
        coroutine.yield()
    end
    return false
end

-- Press and release across separate frames so click-on-release widgets see both edges.
function Test.Click(button)
    button = button or MouseButton.Left
    Test.MouseDown(button)
    Test.Wait(2)
    Test.MouseUp(button)
    Test.Wait(2)
end

function Test.ClickAt(x, y, button)
    Test.MoveMouse(x, y)
    Test.Wait(2)
    Test.Click(button)
end

-- Click a widget by its visible label. Waits for it to appear, since panels
-- often need a frame or two to build after whatever opened them.
function Test.ClickWidget(label, timeout, substring)
    timeout = timeout or 120
    for _ = 1, timeout do
        if Test.MoveToWidget(label, substring) then
            Test.Wait(2)
            Test.Click()
            return true
        end
        coroutine.yield()
    end
    Test.Fail("widget not found: " .. tostring(label))
    error(Test.__ABORT)
end

function Test.WaitForWidget(label, timeout, substring)
    return Test.WaitUntil(function() return Test.WidgetExists(label, substring) end, timeout)
end

function Test.Key(key)
    Test.KeyDown(key)
    Test.Wait(1)
    Test.KeyUp(key)
    Test.Wait(1)
end

function Test.Assert(cond, msg)
    Test.__check()
    if not cond then
        Test.Fail(msg or "assertion failed")
        error(Test.__ABORT)
    end
    return true
end

-- Non-fatal variant: records the failure and carries on.
function Test.Check(cond, msg)
    Test.__check()
    if not cond then Test.Fail(msg or "check failed") end
    return cond and true or false
end

local store = {}
function Test.Expose(name, value) store[name] = value end
function Test.Get(name) return store[name] end
)LUA";

    struct TestRunner::Impl
    {
        sol::state* state = nullptr;
    };

    TestRunner::TestRunner()
    {
        m_Impl = new Impl();
    }

    TestRunner::~TestRunner()
    {
        delete m_Impl;
    }

    void TestRunner::Init(const std::string& scriptPath)
    {
        m_ScriptPath = scriptPath;
        m_Active     = true;
    }

    // m_MousePos is kept in raw Input space so reapplying it is a straight store.
    void TestRunner::SetMousePosRaw(const Vec2& raw)
    {
        m_HasMousePos = true;
        m_MousePos    = raw;
        Input::Get().StoreMousePosition(raw.x, raw.y);
    }

    void TestRunner::SetMousePos(float x, float y)
    {
        Window* w       = Application::Get().GetWindow();
        const float s   = w ? w->GetMousePosScale() : 1.0f;
        const float inv = (s != 0.0f) ? 1.0f / s : 1.0f;
        SetMousePosRaw(Vec2(x * inv, y * inv));
    }

    bool TestRunner::MoveToWidget(const std::string& text, bool substring)
    {
        Vec2 pos;
        if(!UIFindWidgetByText(text.c_str(), &pos, nullptr, substring))
            return false;

        SetMousePosRaw(pos);
        return true;
    }

    void TestRunner::SetMouseButton(int button, bool down)
    {
        if(button < 0 || button >= 32)
            return;

        m_MouseHeld[button] = down;
        Input::Get().SetMouseHeld(InputCode::MouseKey(button), down);
        if(down)
            Input::Get().SetMouseClicked(InputCode::MouseKey(button), true);
    }

    void TestRunner::SetKey(int key, bool down)
    {
        if(key < 0 || key >= 1024)
            return;

        m_KeyHeld[key] = down;
        Input::Get().SetKeyHeld(InputCode::Key(key), down);
        if(down)
            Input::Get().SetKeyPressed(InputCode::Key(key), true);
    }

    void TestRunner::SetScroll(float y, float x)
    {
        Input::Get().SetScrollOffset(y);
        Input::Get().SetScrollOffsetX(x);
    }

    void TestRunner::RequestScreenshot(const std::string& name, bool compare)
    {
        m_PendingShots.PushBack(PendingShot { name, compare });
    }

    void TestRunner::Log(const std::string& msg)
    {
        LINFO("[test] %s", msg.c_str());
    }

    void TestRunner::Fail(const std::string& msg)
    {
        m_Failed = true;
        m_Failures.PushBack(msg);
        LERROR("[test] FAIL: %s", msg.c_str());
    }

    void TestRunner::BindLua(sol::state& state)
    {
        m_Impl->state = &state;

        auto test = state["Test"].get_or_create<sol::table>();

        test["Active"] = m_Active;
        // So scripts can dofile(Test.Dir .. "Common.lua") - tests live outside
        // //Assets, so require's VFS searcher can't reach them.
        test["ScriptPath"] = m_ScriptPath;
        {
            const size_t slash = m_ScriptPath.find_last_of("/\\");
            test["Dir"]        = (slash == std::string::npos) ? std::string() : m_ScriptPath.substr(0, slash + 1);
        }

        test.set_function("MoveMouse", [](float x, float y)
                          { TestRunner::Get().SetMousePos(x, y); });
        // Enum-typed like the rest of the Input API, so scripts write MouseButton.Left / Key.Escape.
        test.set_function("MouseDown", [](InputCode::MouseKey b)
                          { TestRunner::Get().SetMouseButton((int)b, true); });
        test.set_function("MouseUp", [](InputCode::MouseKey b)
                          { TestRunner::Get().SetMouseButton((int)b, false); });
        test.set_function("KeyDown", [](InputCode::Key k)
                          { TestRunner::Get().SetKey((int)k, true); });
        test.set_function("KeyUp", [](InputCode::Key k)
                          { TestRunner::Get().SetKey((int)k, false); });
        test.set_function("Scroll", [](float y, sol::optional<float> x)
                          { TestRunner::Get().SetScroll(y, x.value_or(0.0f)); });
        // Feeds the focused text input the same way the platform char callback does.
        test.set_function("Type", [](const std::string& text)
                          {
                              for(char c : text)
                                  UIProcessKeyTyped(c);
                          });
        // substring = match anywhere in the label, for text that carries live values.
        test.set_function("MoveToWidget", [](const std::string& text, sol::optional<bool> substring) -> bool
                          { return TestRunner::Get().MoveToWidget(text, substring.value_or(false)); });
        test.set_function("WidgetExists", [](const std::string& text, sol::optional<bool> substring) -> bool
                          { return UIFindWidgetByText(text.c_str(), nullptr, nullptr, substring.value_or(false)); });
        test.set_function("Screenshot", [](const std::string& name, sol::optional<bool> compare)
                          { TestRunner::Get().RequestScreenshot(name, compare.value_or(true)); });
        test.set_function("Log", [](const std::string& msg)
                          { TestRunner::Get().Log(msg); });
        test.set_function("Fail", [](const std::string& msg)
                          { TestRunner::Get().Fail(msg); });
        test.set_function("Quit", []()
                          { TestRunner::Get().Finish(true); });
        test.set_function("__check", []()
                          { TestRunner::Get().m_Checks++; });

        state.script(kPrelude);

        if(!m_Active)
            return;

        // Lua 5.4 self-seeds randomly; pin it or anything scattered with math.random
        // moves between runs and golden compares never settle.
        state.script("math.randomseed(20260807)");

        const std::string& root = Application::Get().GetProjectSettings().m_ProjectRoot;
        m_GoldenDir             = root + "Tests/Golden/";
        m_OutDir                = root + "Tests/Out/";

        sol::protected_function start = state["Test"]["__start"];
        auto result                   = start(m_ScriptPath);
        if(!result.valid())
        {
            sol::error err = result;
            Fail(std::string("failed to start test script: ") + err.what());
            Finish(false);
            return;
        }

        sol::optional<std::string> loadErr = result;
        if(loadErr)
        {
            Fail(std::string("failed to load test script: ") + *loadErr);
            Finish(false);
            return;
        }

        LINFO("[test] running %s", m_ScriptPath.c_str());
    }

    void TestRunner::ReapplyStickyInput()
    {
        // The real event pump can clobber injected state (a stray move, a focus
        // change); reassert it every frame so the script stays authoritative.
        if(m_HasMousePos)
            Input::Get().StoreMousePosition(m_MousePos.x, m_MousePos.y);

        for(int i = 0; i < 32; i++)
        {
            if(m_MouseHeld[i])
                Input::Get().SetMouseHeld(InputCode::MouseKey(i), true);
        }

        for(int i = 0; i < 1024; i++)
        {
            if(m_KeyHeld[i])
                Input::Get().SetKeyHeld(InputCode::Key(i), true);
        }
    }

    void TestRunner::Step()
    {
        if(!m_Active)
            return;

        if(m_Finished)
        {
            // Init() flips the app to Running after we may have already asked to
            // close (script failed to load), so keep asserting it.
            Application::Get().SetAppState(AppState::Closing);
            return;
        }

        ReapplyStickyInput();

        m_Frame++;
        if(m_Frame > m_TimeoutFrames)
        {
            Fail("test timed out after " + std::to_string(m_TimeoutFrames) + " frames");
            Finish(false);
            return;
        }

        Resume();
    }

    void TestRunner::Resume()
    {
        sol::state& state = *m_Impl->state;

        sol::protected_function step = state["Test"]["__step"];
        auto result                  = step();
        if(!result.valid())
        {
            sol::error err = result;
            Fail(std::string("test error: ") + err.what());
            Finish(false);
            return;
        }

        std::string status = result.get<std::string>(0);
        if(status == "error")
        {
            sol::optional<std::string> msg = result.get<sol::optional<std::string>>(1);
            Fail(std::string("test error: ") + (msg ? *msg : "unknown"));
            Finish(false);
            return;
        }

        if(status == "aborted")
        {
            Finish(false);
            return;
        }

        if(status == "dead")
        {
            // Let any screenshot queued on the final frame resolve first.
            if(m_PendingShots.Empty())
                Finish(!m_Failed);
        }
    }

    void TestRunner::Finish(bool passed)
    {
        if(m_Finished)
            return;

        m_Finished = true;

        if(passed && !m_Failed)
        {
            LINFO("[test] PASS (%d checks, %d frames)", m_Checks, m_Frame);
            Application::SetExitCode(0);
        }
        else
        {
            LERROR("[test] FAILED (%d failures, %d frames)", (int)m_Failures.Size(), m_Frame);
            Application::SetExitCode(1);
        }

        WriteReport();
        Application::Get().SetAppState(AppState::Closing);
    }

    void TestRunner::WriteReport()
    {
        if(m_ReportPath.empty())
            return;

        std::error_code ec;
        auto fsPath = std::filesystem::path(m_ReportPath);
        if(fsPath.has_parent_path())
            std::filesystem::create_directories(fsPath.parent_path(), ec);

        std::ofstream out(m_ReportPath);
        if(!out)
        {
            LERROR("[test] could not write report: %s", m_ReportPath.c_str());
            return;
        }

        out << "script: " << m_ScriptPath << "\n";
        out << "result: " << ((!m_Failed) ? "PASS" : "FAIL") << "\n";
        out << "frames: " << m_Frame << "\n";
        out << "checks: " << m_Checks << "\n";
        for(size_t i = 0; i < m_Failures.Size(); i++)
            out << "failure: " << m_Failures[i] << "\n";
    }

    void TestRunner::ServiceScreenshots(bool canCapture)
    {
        if(!m_Active || m_PendingShots.Empty())
            return;

        for(size_t i = 0; i < m_PendingShots.Size(); i++)
        {
            const PendingShot& shot  = m_PendingShots[i];
            const std::string actual = m_OutDir + shot.name + ".png";

            if(canCapture)
            {
                Graphics::Renderer::GetRenderer()->SaveScreenshot(
                    actual, Application::Get().GetWindow()->GetSwapChain()->GetCurrentImage());
            }

            if(shot.compare)
                CompareAgainstGolden(shot.name, actual);
            else
                LINFO("[test] captured (not compared): %s", actual.c_str());
        }

        m_PendingShots.Clear();
    }

    bool TestRunner::CompareAgainstGolden(const std::string& name, const std::string& actualPath)
    {
        const std::string goldenPath = m_GoldenDir + name + ".png";

        std::error_code ec;
        if(m_UpdateGolden)
        {
            std::filesystem::create_directories(m_GoldenDir, ec);
            std::filesystem::copy_file(actualPath, goldenPath,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            if(ec)
            {
                Fail("could not update golden '" + name + "': " + ec.message());
                return false;
            }
            LINFO("[test] golden updated: %s", goldenPath.c_str());
            return true;
        }

        int aw, ah, an, gw, gh, gn;
        uint8_t* a = stbi_load(actualPath.c_str(), &aw, &ah, &an, 4);
        if(!a)
        {
            Fail("screenshot '" + name + "' was not written");
            return false;
        }

        uint8_t* g = stbi_load(goldenPath.c_str(), &gw, &gh, &gn, 4);
        if(!g)
        {
            stbi_image_free(a);
            Fail("no golden for '" + name + "' - run with --test-update-golden first");
            return false;
        }

        if(aw != gw || ah != gh)
        {
            Fail("screenshot '" + name + "' size " + std::to_string(aw) + "x" + std::to_string(ah)
                 + " != golden " + std::to_string(gw) + "x" + std::to_string(gh));
            stbi_image_free(a);
            stbi_image_free(g);
            return false;
        }

        const size_t pixels = (size_t)aw * (size_t)ah;
        size_t differing    = 0;

        TDArray<uint8_t> diff;
        diff.Resize(pixels * 4);

        for(size_t p = 0; p < pixels; p++)
        {
            const size_t o = p * 4;
            int maxDelta   = 0;
            for(int c = 0; c < 3; c++)
                maxDelta = Maths::Max(maxDelta, Maths::Abs((int)a[o + c] - (int)g[o + c]));

            const bool bad = maxDelta > CHANNEL_THRESHOLD;
            if(bad)
                differing++;

            // Offending pixels pop red, everything else dims to grey.
            if(bad)
            {
                diff[o + 0] = 255;
                diff[o + 1] = 0;
                diff[o + 2] = 0;
            }
            else
            {
                const uint8_t grey = (uint8_t)(((int)a[o] + a[o + 1] + a[o + 2]) / 6);
                diff[o + 0] = grey;
                diff[o + 1] = grey;
                diff[o + 2] = grey;
            }
            diff[o + 3] = 255;
        }

        const float fraction = pixels > 0 ? (float)differing / (float)pixels : 0.0f;
        m_Checks++;

        bool passed = fraction <= FAIL_FRACTION;
        if(!passed)
        {
            const std::string diffPath = m_OutDir + name + "_diff.png";
            ImageExport::SavePNG(diffPath, (uint32_t)aw, (uint32_t)ah, diff.Data());
            char buf[256];
            snprintf(buf, sizeof(buf), "screenshot '%s' differs from golden (%.3f%% of pixels) - see %s",
                     name.c_str(), fraction * 100.0f, diffPath.c_str());
            Fail(buf);
        }

        stbi_image_free(a);
        stbi_image_free(g);
        return passed;
    }
}
