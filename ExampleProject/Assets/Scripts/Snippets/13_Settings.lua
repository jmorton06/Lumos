-- ============================================================================
-- 13_Settings — slider/toggle panels wired to persisted save
-- ============================================================================

-- ===== Snippet: Volume + mouse sensitivity sliders ==========================
local volume      = 0.8
local sensitivity = 1.0
local fullscreen  = false
local vsync       = true

function OnUpdate(dt)
    UIBeginPanel("Settings", SizeKind.Pixels, 360, SizeKind.ChildSum, 1,
        WidgetFlags.StackVertically | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
        UILabel("title", "Settings")
        UISeparator()
        volume       = UISlider("Volume",      volume,      0.0, 1.0)
        sensitivity  = UISlider("Sensitivity", sensitivity, 0.1, 4.0)
        fullscreen   = UIToggle("Fullscreen",  fullscreen)
        vsync        = UIToggle("VSync",       vsync)
        if UIButton("Apply").clicked then Log.Info("apply settings") end
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Persist settings via Save.lua ===============================
local Save = require("game/Save")
local volume, vsync

function OnInit()
    Save.Load()
    volume = Save.GetSetting("volume", 0.8)
    vsync  = Save.GetSetting("vsync", true)
end

function OnUpdate(dt)
    UIBeginPanel("Settings", SizeKind.Pixels, 320, SizeKind.ChildSum, 1,
        WidgetFlags.StackVertically | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
        local v2 = UISlider("Volume", volume, 0, 1)
        if v2 ~= volume then volume = v2; Save.SetSetting("volume", volume) end
        local ys = UIToggle("VSync", vsync)
        if ys ~= vsync then vsync = ys; Save.SetSetting("vsync", vsync) end
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Keybinding picker (capture next key) ========================
local jumpKey = Key.Space
local capturing = false
function OnUpdate(dt)
    if capturing then
        for k = 32, 348 do  -- scan key codes; replace with explicit list as needed
            if Input.GetKeyPressed(k) then
                jumpKey = k
                capturing = false
                break
            end
        end
    end
    UIBeginPanel("Keys", SizeKind.Pixels, 280, SizeKind.ChildSum, 1, WidgetFlags.StackVertically)
        UILabel("jump", "Jump: " .. tostring(jumpKey))
        if UIButton("Rebind").clicked then capturing = true end
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Dropdown-style toggle group (pick one) ======================
local quality = "Medium"
local options = { "Low", "Medium", "High", "Ultra" }
function OnUpdate(dt)
    UIBeginPanel("Quality", SizeKind.Pixels, 260, SizeKind.ChildSum, 1, WidgetFlags.StackVertically)
        UILabel("title", "Quality")
        for _, opt in ipairs(options) do
            local picked = UIToggle(opt, quality == opt)
            if picked and quality ~= opt then quality = opt end
        end
    UIEndPanel()
end
-- ============================================================================
