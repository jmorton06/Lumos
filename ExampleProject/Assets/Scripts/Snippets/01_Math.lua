-- ============================================================================
-- 01_Math — clamp / lerp / damp / easing / vector helpers
-- ============================================================================
-- The engine ships util/Math.lua already (Clamp, Lerp, LerpVec3, Damp, DampVec3,
-- SmoothStep, Sign, DistXZ, Length, Normalize, DegToRad, RadToDeg).
-- Use:  local Math = require("util/Math")

-- ===== Snippet: Lerp + Damp (frame-rate independent smoothing) ==============
local Math = require("util/Math")
local current = 0.0
function OnUpdate(dt)
    local target = Input.GetKeyHeld(Key.W) and 10.0 or 0.0
    current = Math.Damp(current, target, 6.0, dt)  -- larger = snappier
end
-- ============================================================================


-- ===== Snippet: Smoothstep + custom ease ====================================
local function EaseOutBack(t)
    local c1 = 1.70158
    local c3 = c1 + 1.0
    local x  = t - 1.0
    return 1.0 + c3 * x * x * x + c1 * x * x
end

function OnUpdate(dt)
    local t = math.min(1.0, (os.clock() % 2.0) / 1.0)
    local k = EaseOutBack(t)              -- 0..1 with overshoot bounce
end
-- ============================================================================


-- ===== Snippet: Built-in easing curves ======================================
-- Globally exposed: SineIn/Out/InOut, ExponentialIn/Out/InOut,
-- ElasticIn/Out/InOut, AnimateToTarget(current, target, speed)
local v = 0.0
function OnUpdate(dt)
    v = AnimateToTarget(v, 10.0, 4.0)
end
-- ============================================================================


-- ===== Snippet: Vec3 Dot / Cross (added bindings) ===========================
-- right = up x forward, etc.
local up      = Vec3.new(0, 1, 0)
local forward = Vec3.new(0, 0, 1)
local right   = Vec3.Cross(up, forward)         -- (1,0,0)
local dot     = Vec3.Dot(up, forward)           -- 0
-- ============================================================================


-- ===== Snippet: Vec3 lerp + MoveTowards (added bindings) ====================
local a = Vec3.new(0, 0, 0)
local b = Vec3.new(10, 5, 0)
local mid     = Vec3.Lerp(a, b, 0.5)
local stepped = Vec3.MoveTowards(a, b, 1.0)     -- exactly 1u toward b
-- ============================================================================


-- ===== Snippet: Quat from look-at + slerp (added binding) ===================
local q0 = QuatLookAt(Vec3.new(0,0,0), Vec3.new(1,0,0))
local q1 = QuatLookAt(Vec3.new(0,0,0), Vec3.new(0,0,1))
local q  = QuatSlerp(q0, q1, 0.5)               -- halfway rotation
local v  = q:Transform(Vec3.new(1,0,0))         -- rotate vec by quat
-- ============================================================================


-- ===== Snippet: Clamp + remap ===============================================
local Math = require("util/Math")
local function Remap(v, a0, a1, b0, b1)
    local t = (v - a0) / (a1 - a0)
    return b0 + Math.Clamp(t, 0, 1) * (b1 - b0)
end

local hp = 75.0
local pct = Remap(hp, 0, 100, 0, 1)  -- 0.75
-- ============================================================================


-- ===== Snippet: Angle helpers ===============================================
local function AngleBetween(a, b)
    local da = a.x * b.x + a.y * b.y + a.z * b.z
    local la = math.sqrt(a.x*a.x + a.y*a.y + a.z*a.z)
    local lb = math.sqrt(b.x*b.x + b.y*b.y + b.z*b.z)
    if la < 1e-6 or lb < 1e-6 then return 0 end
    return math.acos(math.max(-1, math.min(1, da / (la * lb))))
end
-- ============================================================================


-- ===== Snippet: Sin/cos based wobble ========================================
local _t = 0.0
function OnUpdate(dt)
    _t = _t + dt
    local wobble = math.sin(_t * 6.28 * 2.0) * 0.5   -- 2 Hz, amplitude 0.5
end
-- ============================================================================
