-- ============================================================================
-- 04_Movement — translate, rotate, follow, look-at
-- ============================================================================
local Math = require("util/Math")

-- ===== Snippet: Sin-wave bobbing ============================================
local _t, _trans, _base
function OnInit()
    _trans = LuaComponent:GetCurrentEntity():GetOrAddTransform()
    _base  = _trans.LocalPosition
    _t     = 0.0
end
function OnUpdate(dt)
    _t = _t + dt
    _trans:SetLocalPosition(Vec3.new(_base.x, _base.y + math.sin(_t * 2.0) * 0.5, _base.z))
end
-- ============================================================================


-- ===== Snippet: Constant rotation ===========================================
local _t = 0.0
function OnUpdate(dt)
    _t = _t + dt
    local q = Quat.new(0, _t * 90.0, 0)         -- yaw at 90 deg/s
    LuaComponent:GetCurrentEntity():GetTransform():SetLocalOrientation(q)
end
-- ============================================================================


-- ===== Snippet: Smooth follow (damped) ======================================
-- Attach to follower; supply 'target' entity name.
local follower, target
function OnInit()
    follower = LuaComponent:GetCurrentEntity():GetTransform()
    target   = GetEntityByName(scene, "Player")
end
function OnUpdate(dt)
    if not (target and target:Valid()) then return end
    local goal = target:GetTransform():GetWorldPosition()
    local cur  = follower.LocalPosition
    local next = Math.DampVec3(cur, goal + Vec3.new(0, 5, -8), 3.0, dt)
    follower:SetLocalPosition(next)
end
-- ============================================================================


-- ===== Snippet: MoveTowards (constant speed) ================================
local Math = require("util/Math")
local t, target = nil, Vec3.new(10, 0, 0)
function OnInit() t = LuaComponent:GetCurrentEntity():GetTransform() end
function OnUpdate(dt)
    local cur = t.LocalPosition
    t:SetLocalPosition(Vec3.MoveTowards(cur, target, 4.0 * dt))
end
-- ============================================================================


-- ===== Snippet: Look at point (orient forward toward target) ================
local t
function OnInit() t = LuaComponent:GetCurrentEntity():GetTransform() end
function OnUpdate(dt)
    local me   = t:GetWorldPosition()
    local goal = Vec3.new(0, 0, 0)
    t:SetLocalOrientation(QuatLookAt(me, goal))
end
-- ============================================================================


-- ===== Snippet: Patrol between waypoints ====================================
local Math = require("util/Math")
local pts = { Vec3.new(0,0,0), Vec3.new(10,0,0), Vec3.new(10,0,10), Vec3.new(0,0,10) }
local i, t = 1, nil
function OnInit() t = LuaComponent:GetCurrentEntity():GetTransform() end
function OnUpdate(dt)
    local cur  = t.LocalPosition
    local goal = pts[i]
    local nxt  = Vec3.MoveTowards(cur, goal, 3.0 * dt)
    t:SetLocalPosition(nxt)
    if Math.Length(goal - nxt) < 0.05 then
        i = (i % #pts) + 1
    end
end
-- ============================================================================


-- ===== Snippet: Strafe orbit around a point =================================
local centre = Vec3.new(0, 0, 0)
local R, ang = 5.0, 0.0
local t
function OnInit() t = LuaComponent:GetCurrentEntity():GetTransform() end
function OnUpdate(dt)
    ang = ang + dt * 1.0
    local p = Vec3.new(centre.x + math.cos(ang) * R, centre.y, centre.z + math.sin(ang) * R)
    t:SetLocalPosition(p)
    t:SetLocalOrientation(QuatLookAt(p, centre))
end
-- ============================================================================


-- ===== Snippet: Snap-to-ground via terrain ==================================
local t
function OnInit() t = LuaComponent:GetCurrentEntity():GetTransform() end
function OnUpdate(dt)
    local p = t.LocalPosition
    local h = TerrainHeightAt(p.x, p.z)
    if h then t:SetLocalPosition(Vec3.new(p.x, h + 0.5, p.z)) end
end
-- ============================================================================
