-- ============================================================================
-- 12_Menus — main menu, pause menu, confirm dialog
-- ============================================================================

-- ===== Snippet: Main menu (centered) ========================================
local menuState = "main"   -- "main" | "options" | "play"
function OnUpdate(dt)
    if menuState ~= "main" then return end
    UIBeginPanel("MainMenu", SizeKind.Pixels, 320, SizeKind.ChildSum, 1,
        WidgetFlags.StackVertically | WidgetFlags.CentreChildrenX | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
        UIPushStyle(StyleVar.FontSize, 36)
        UILabel("title", "My Game")
        UIPopStyle()
        UISpacer(16)
        if UIButton("Play").clicked        then menuState = "play"    end
        if UIButton("Options").clicked     then menuState = "options" end
        if UIButton("Quit").clicked        then ExitApp()             end
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Pause menu (toggle on Esc) ==================================
local paused = false
function OnUpdate(dt)
    if Input.GetKeyPressed(Key.Escape) then paused = not paused end
    if not paused then return end
    UIBeginPanel("Pause", SizeKind.Pixels, 280, SizeKind.ChildSum, 1,
        WidgetFlags.StackVertically | WidgetFlags.CentreChildrenX | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
        UILabel("title", "Paused")
        UISeparator()
        if UIButton("Resume").clicked   then paused = false        end
        if UIButton("Restart").clicked  then SwitchSceneByName(GetCurrentScene():GetSceneName()) end
        if UIButton("Main Menu").clicked then SwitchSceneByName("MainMenu") end
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Confirm dialog (yes/no callback) ============================
local confirm = nil   -- { msg = ..., onYes = func, onNo = func }
local function Ask(msg, onYes, onNo)
    confirm = { msg = msg, onYes = onYes, onNo = onNo }
end
function OnUpdate(dt)
    if not confirm then return end
    UIBeginPanel("Confirm", SizeKind.Pixels, 280, SizeKind.ChildSum, 1,
        WidgetFlags.StackVertically | WidgetFlags.CentreChildrenX | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
        UILabel("msg", confirm.msg)
        UISpacer(8)
        UIBeginPanel("row", SizeKind.ChildSum, 1, SizeKind.ChildSum, 1, WidgetFlags.StackHorizontally)
            if UIButton("Yes").clicked then
                if confirm.onYes then confirm.onYes() end
                confirm = nil
            end
            if UIButton("No").clicked then
                if confirm.onNo then confirm.onNo() end
                confirm = nil
            end
        UIEndPanel()
    UIEndPanel()
end
-- Usage:  Ask("Quit to desktop?", function() ExitApp() end, nil)
-- ============================================================================


-- ===== Snippet: Tab bar (sub-menus) =========================================
local tab = "play"
function OnUpdate(dt)
    UIBeginPanel("Tabs", SizeKind.Pixels, 480, SizeKind.Pixels, 360,
        WidgetFlags.StackVertically | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
        UIBeginPanel("tabRow", SizeKind.ChildSum, 1, SizeKind.ChildSum, 1, WidgetFlags.StackHorizontally)
            if UIButton("Play").clicked     then tab = "play"     end
            if UIButton("Options").clicked  then tab = "options"  end
            if UIButton("About").clicked    then tab = "about"    end
        UIEndPanel()
        UISeparator()
        if tab == "play"    then UILabel("p", "press Start") end
        if tab == "options" then UILabel("o", "volume / graphics ...") end
        if tab == "about"   then UILabel("a", "credits text") end
    UIEndPanel()
end
-- ============================================================================
