-- ============================================================================
-- 11_HUD — score, lives, timer overlay, health bar
-- ============================================================================
-- UI funcs are immediate-mode. Call from OnUpdate or a dedicated UpdateUI().

-- ===== Snippet: Score + lives top-left ======================================
local score = 0
local lives = 3
function OnUpdate(dt)
    UIBeginOverlay("HUD", SizeKind.ChildSum, 1, SizeKind.ChildSum, 1,
        WidgetFlags.StackVertically | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.TopLeft, 16, 16)
        UILabel("score", "Score: " .. score)
        UILabel("lives", "Lives: " .. lives)
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Countdown timer =============================================
local timeLeft = 60.0
function OnUpdate(dt)
    timeLeft = math.max(0, timeLeft - dt)
    UIBeginOverlay("Timer", SizeKind.ChildSum, 1, SizeKind.ChildSum, 1,
        WidgetFlags.StackHorizontally | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.TopCenter, 0, 8)
        UIPushStyle(StyleVar.FontSize, 28)
        UILabel("t", string.format("%02d:%02d", math.floor(timeLeft / 60), math.floor(timeLeft) % 60))
        UIPopStyle()
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Health bar (progress bar) ===================================
local hp, hpMax = 80, 100
function OnUpdate(dt)
    UIBeginOverlay("HP", SizeKind.Pixels, 220, SizeKind.ChildSum, 1, WidgetFlags.StackVertically)
        UIWindowAnchor(UIAnchor.BottomLeft, 16, 16)
        UILabel("hpLabel", "HP " .. hp .. "/" .. hpMax)
        UIProgressBar("hpBar", hp / hpMax, 200, 14)
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Crosshair (centered dot) ====================================
function OnUpdate(dt)
    UIBeginOverlay("Crosshair", SizeKind.Pixels, 6, SizeKind.Pixels, 6,
        WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Floating combo popup (decays) ===============================
local popup = { text = "", t = 0.0 }
local function ShowCombo(s)
    popup.text = s
    popup.t = 1.0
end
function OnUpdate(dt)
    if popup.t > 0 then
        popup.t = popup.t - dt
        UIBeginOverlay("Combo", SizeKind.ChildSum, 1, SizeKind.ChildSum, 1)
            UIWindowAnchor(UIAnchor.TopCenter, 0, 80)
            local alpha = math.min(1.0, popup.t * 2.0)
            UIPushStyle(StyleVar.TextColor, Vec4.new(1, 0.8, 0.1, alpha))
            UIPushStyle(StyleVar.FontSize, 40)
            UILabel("combo", popup.text)
            UIPopStyle()
            UIPopStyle()
        UIEndPanel()
    end
end
-- ============================================================================


-- ===== Snippet: Anchored hint bottom-right ==================================
function OnUpdate(dt)
    UIBeginOverlay("Hint", SizeKind.ChildSum, 1, SizeKind.ChildSum, 1, WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.BottomRight, 16, 16)
        UILabel("hint", "Press [E] to interact")
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Toast / notification queue ==================================
local _toasts = {}
local function Toast(msg, secs)
    table.insert(_toasts, { msg = msg, ttl = secs or 2.0 })
end
function OnUpdate(dt)
    for i = #_toasts, 1, -1 do
        _toasts[i].ttl = _toasts[i].ttl - dt
        if _toasts[i].ttl <= 0 then table.remove(_toasts, i) end
    end
    if #_toasts == 0 then return end
    UIBeginOverlay("Toasts", SizeKind.ChildSum, 1, SizeKind.ChildSum, 1,
        WidgetFlags.StackVertically | WidgetFlags.DrawBackground)
        UIWindowAnchor(UIAnchor.TopRight, 16, 16)
        for i, t in ipairs(_toasts) do
            UILabel("toast"..i, t.msg)
        end
    UIEndPanel()
end
-- ============================================================================


-- ===== Snippet: Full-screen flash (red on damage) ===========================
local flash = 0.0
local function DamageFlash() flash = 0.5 end
function OnUpdate(dt)
    if flash > 0 then
        flash = math.max(0, flash - dt)
        UIBeginOverlay("Flash", SizeKind.PercentOfViewport, 1, SizeKind.PercentOfViewport, 1, 0)
            UIWindowAnchor(UIAnchor.MiddleCenter, 0, 0)
            UIPushStyle(StyleVar.BackgroundColor, Vec4.new(1, 0, 0, flash))
            UILabel("flash", "")
            UIPopStyle()
        UIEndPanel()
    end
end
-- ============================================================================
