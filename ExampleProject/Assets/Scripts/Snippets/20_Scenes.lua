-- ============================================================================
-- 20_Scenes — switch / restart / fade
-- ============================================================================

-- ===== Snippet: Switch by name / index ======================================
function OnUpdate(dt)
    if Input.GetKeyPressed(Key.N) then SwitchScene() end             -- next
    if Input.GetKeyPressed(Key.M) then SwitchSceneByName("MainMenu") end
    if Input.GetKeyPressed(Key.K) then SwitchSceneByIndex(0)         end
end
-- ============================================================================


-- ===== Snippet: Restart current scene =======================================
local function Restart()
    SwitchSceneByName(GetCurrentScene():GetSceneName())
end
-- ============================================================================


-- ===== Snippet: Fade-to-black before switch =================================
local Math = require("util/Math")
local fading, fadeT, FADE_SEC = false, 0.0, 0.5
local nextScene = nil

local function FadeOutTo(name)
    nextScene = name
    fading = true
    fadeT = 0.0
end

function OnUpdate(dt)
    if not fading then return end
    fadeT = math.min(FADE_SEC, fadeT + dt)
    local a = fadeT / FADE_SEC
    UIBeginOverlay("Fade", SizeKind.PercentOfViewport, 1, SizeKind.PercentOfViewport, 1, 0)
        UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
        UIPushStyle(StyleVar.BackgroundColor, Vec4.new(0, 0, 0, a))
        UILabel("fade", "")
        UIPopStyle()
    UIEndPanel()
    if fadeT >= FADE_SEC then
        fading = false
        if nextScene then SwitchSceneByName(nextScene); nextScene = nil end
    end
end
-- ============================================================================


-- ===== Snippet: Quit the app ================================================
if Input.GetKeyPressed(Key.Escape) then ExitApp() end
-- ============================================================================
