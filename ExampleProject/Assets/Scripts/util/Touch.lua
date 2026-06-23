-- Portrait split-screen touch helpers.
-- Works on both iOS (pan gesture) and macOS/desktop (mouse drag).
-- Pad samples cursor position each frame and computes its own frame-delta,
-- regardless of which input source is providing the position.

local M = {}

function M.ScreenSize()
    return Input.GetScreenSize()
end

local function _IsLeft(pos, screen)
    return pos.x < screen.x * 0.5
end

function M.IsLeftHalf(pos)
    return _IsLeft(pos, Input.GetScreenSize())
end

function M.IsRightHalf(pos)
    return not M.IsLeftHalf(pos)
end

-- Pad: tracks a single drag inside its half of the screen.
local Pad = {}
Pad.__index = Pad

function M.NewPad(side) -- "left" or "right"
    return setmetatable({
        side = side,
        active = false,
        startPos = Vec2.new(0, 0),
        lastPos = Vec2.new(0, 0),
        delta = Vec2.new(0, 0),
        total = Vec2.new(0, 0),
        released = false,
    }, Pad)
end

-- Per-pad "is the right input source down" check.
-- Touch (iOS single-finger) reports as MouseButton.Left + a live MousePosition
-- updated in touchesMoved. Pan gesture (2-finger) freezes position and only
-- delivers a translation delta, so we can't rely on it for the charge drag.
-- Rule: split MouseButton.Left by screen half (works for both touch and a
-- left-mouse drag on desktop). RMB anywhere still maps to rightPad so a mac
-- trackpad two-finger click acts as the right pad shortcut.
local function _IsDownFor(side)
    -- RMB anywhere → rightPad (desktop trackpad two-finger click).
    if side == "right" and Input.GetMouseHeld(MouseButton.Right) then return true end

    local pos
    if Input.GetMouseHeld(MouseButton.Left) or Input.GetPanActive() then
        pos = Input.GetMousePosition()
    elseif Input.GetLongPressActive() then
        pos = Input.GetLongPressPosition()
    end
    if not pos then return false end

    local left = _IsLeft(pos, Input.GetScreenSize())
    return (side == "left") == left
end

-- Best-effort current pointer position (screen pixels).
-- iOS pan gesture stores translation relative to gesture start; we ignore that and
-- read MousePosition which iOSOS.mm also updates on touch events.
local function _PointerPos()
    return Input.GetMousePosition()
end

function Pad:_InRegion(pos, screen)
    -- _IsDownFor already handles region filtering for every input source.
    return true
end

function Pad:Update()
    self.released = false
    self.delta = Vec2.new(0, 0)

    local down = _IsDownFor(self.side)
    if down then
        local pos = _PointerPos()
        local screen = Input.GetScreenSize()
        if not self.active then
            if self:_InRegion(pos, screen) then
                self.active = true
                self.startPos = pos
                self.lastPos = pos
                self.total = Vec2.new(0, 0)
            end
        else
            self.delta = Vec2.new(pos.x - self.lastPos.x, pos.y - self.lastPos.y)
            self.total = Vec2.new(pos.x - self.startPos.x, pos.y - self.startPos.y)
            self.lastPos = pos
        end
    else
        if self.active then self.released = true end
        self.active = false
        self.total = Vec2.new(0, 0)
    end
end

function Pad:Delta()    return self.delta end
function Pad:Active()   return self.active end
function Pad:Released() return self.released end
function Pad:TotalY()   return self.total.y end
function Pad:TotalX()   return self.total.x end

return M
