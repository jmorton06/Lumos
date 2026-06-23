-- ============================================================================
-- 02_Input — keyboard, mouse, gamepad, touch
-- ============================================================================

-- ===== Snippet: Keyboard held / pressed (edge) ==============================
local _spacePrev = false
function OnUpdate(dt)
    if Input.GetKeyHeld(Key.W) then
        -- continuous
    end

    local space = Input.GetKeyHeld(Key.Space)
    if space and not _spacePrev then
        Log.Info("space pressed (edge)")
    end
    _spacePrev = space
end
-- ============================================================================


-- ===== Snippet: Built-in edge check (single-shot) ===========================
function OnUpdate(dt)
    if Input.GetKeyPressed(Key.R) then
        Log.Info("R pressed once")
    end
end
-- ============================================================================


-- ===== Snippet: Mouse buttons + position ====================================
function OnUpdate(dt)
    if Input.GetMouseClicked(MouseButton.Left) then
        local p = Input.GetMousePosition()
        Log.Info(string.format("click at %.1f %.1f", p.x, p.y))
    end

    if Input.GetMouseHeld(MouseButton.Right) then
        -- drag
    end

    local scroll = Input.GetScrollOffset()
    if math.abs(scroll) > 0.01 then
        -- zoom
    end
end
-- ============================================================================


-- ===== Snippet: Gamepad (controller 0) ======================================
function OnUpdate(dt)
    local lx = Input.GetControllerAxis(0, 0)   -- left stick X
    local ly = Input.GetControllerAxis(0, 1)   -- left stick Y
    if Input.IsControllerButtonPressed(0, 0) then  -- A / cross
        Log.Info("A button")
    end
    local name = Input.GetControllerName(0)
end
-- ============================================================================


-- ===== Snippet: Mouse-only WASD walk (camera-relative) ======================
local Math = require("util/Math")
local SPEED = 5.0
local _t

function OnInit()
    _t = LuaComponent:GetCurrentEntity():GetOrAddTransform()
end

function OnUpdate(dt)
    local f, r = Vec3.new(0,0,1), Vec3.new(1,0,0)
    local move = Vec3.new(0,0,0)
    if Input.GetKeyHeld(Key.W) then move = move + f end
    if Input.GetKeyHeld(Key.S) then move = move - f end
    if Input.GetKeyHeld(Key.D) then move = move + r end
    if Input.GetKeyHeld(Key.A) then move = move - r end
    if Math.Length(move) > 0.001 then
        move = Math.Normalize(move) * (SPEED * dt)
        local p = _t.LocalPosition
        _t:SetLocalPosition(Vec3.new(p.x + move.x, p.y, p.z + move.z))
    end
end
-- ============================================================================


-- ===== Snippet: Touch pad (left/right screen halves) ========================
local Touch = require("util/Touch")
local leftPad, rightPad

function OnInit()
    leftPad  = Touch.NewPad("left")
    rightPad = Touch.NewPad("right")
end

function OnUpdate(dt)
    leftPad:Update()
    rightPad:Update()

    if rightPad:Active() then
        local d = rightPad:Delta()       -- per-frame movement Vec2
        -- rotate camera, etc.
    end

    if rightPad:Released() then
        Log.Info("release shot")
    end
end
-- ============================================================================


-- ===== Snippet: Pinch zoom (mobile / trackpad) ==============================
local zoom = 5.0
function OnUpdate(dt)
    if Input.GetPinchActive() then
        zoom = zoom * (1.0 + (1.0 - Input.GetPinchScale()) * 0.1)
    end
end
-- ============================================================================


-- ===== Snippet: Long-press to open context menu =============================
function OnUpdate(dt)
    if Input.GetLongPressActive() then
        local p = Input.GetLongPressPosition()
        -- show menu near p
    end
end
-- ============================================================================
