-- Smoke test for the in-game debug overlay (command palette + HUD panels).
-- Run: Runtime --project=<ExampleProject.lmproj> --test=<abs path to this file>

Test.Log("debug overlay smoke test")
Test.Wait(10)

Debug.SetEnabled(true)

local ran = false
Debug.RegisterCommand("Smoke Test Command", "Test", function() ran = true end)

Debug.Open()
Test.Wait(3)
Test.Assert(Debug.IsOpen(), "palette should be open")
Test.Assert(Test.WaitForWidget("Search commands", 60, true), "search field should be visible")

-- Fuzzy match: non-contiguous letters across category + name.
Test.Type("smoke")
Test.Wait(3)
Test.Assert(Test.WaitForWidget("Smoke Test Command", 60), "fuzzy search should surface the command")

Test.ClickWidget("Smoke Test Command")
Test.Wait(3)
Test.Assert(ran, "clicking the row should run the command")
Test.Assert(not Debug.IsOpen(), "running an action should close the palette")

-- Toggle commands flip live state and leave the palette up.
Debug.Open()
Test.Wait(3)
Test.Type("shadows")
Test.Wait(3)
Test.Assert(Test.WaitForWidget("Shadows", 60), "built-in render toggles should be listed")
local before = Render.GetBool("Shadows")
Test.ClickWidget("Shadows")
Test.Wait(3)
Test.Assert(Render.GetBool("Shadows") ~= before, "toggle row should flip the render setting")
Test.Assert(Debug.IsOpen(), "toggles should keep the palette open")
Test.ClickWidget("Shadows")
Test.Wait(2)

Test.Key(Key.Escape)
Test.Wait(3)
Test.Assert(not Debug.IsOpen(), "escape should close the palette")

-- HUD panels
Debug.ShowPanel("Stats", true)
Test.Wait(5)
Test.Assert(Test.WidgetExists("FPS", true), "stats HUD should show a frame rate")
Test.Assert(Debug.IsPanelVisible("Stats"), "panel state should read back")

Debug.ShowPanel("Console", true)
Test.Wait(5)
Test.Assert(Test.WidgetExists("Console"), "console panel should be visible")

Debug.Run("Hide All Overlays")
Test.Wait(5)
Test.Assert(not Debug.IsPanelVisible("Stats"), "Hide All Overlays should clear panels")

Debug.Unregister("Smoke Test Command")
Test.Log("debug overlay smoke test passed")
Test.Quit()
