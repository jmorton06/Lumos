-- ============================================================================
-- 23_AI — patrol, chase, flee, line-of-sight
-- ============================================================================
local Math = require("util/Math")

-- ===== Snippet: Patrol along waypoints ======================================
local waypoints = { Vec3.new(0,0,0), Vec3.new(8,0,0), Vec3.new(8,0,8), Vec3.new(0,0,8) }
local idx, _t = 1, nil
function OnInit() _t = LuaComponent:GetCurrentEntity():GetTransform() end
function OnUpdate(dt)
    local cur = _t.LocalPosition
    local goal = waypoints[idx]
    _t:SetLocalPosition(Vec3.MoveTowards(cur, goal, 2.0 * dt))
    _t:SetLocalOrientation(QuatLookAt(cur, goal))
    if Math.Length(cur - goal) < 0.1 then idx = (idx % #waypoints) + 1 end
end
-- ============================================================================


-- ===== Snippet: Chase target if close, else patrol ==========================
local Math = require("util/Math")
local t, target = nil, nil
local AGGRO_RADIUS = 6.0

function OnInit()
    t = LuaComponent:GetCurrentEntity():GetTransform()
    target = GetEntityByName(scene, "Player")
end

function OnUpdate(dt)
    if not (target and target:Valid()) then return end
    local me = t.LocalPosition
    local p  = target:GetTransform():GetWorldPosition()
    if Math.Length(p - me) < AGGRO_RADIUS then
        t:SetLocalPosition(Vec3.MoveTowards(me, p, 3.5 * dt))
        t:SetLocalOrientation(QuatLookAt(me, p))
    end
end
-- ============================================================================


-- ===== Snippet: Flee target if too close ====================================
local Math = require("util/Math")
function OnUpdate(dt)
    if not (target and target:Valid()) then return end
    local me = t.LocalPosition
    local p  = target:GetTransform():GetWorldPosition()
    local d  = me - p
    if Math.Length(d) < 4.0 then
        local away = me + Math.Normalize(d) * (4.0 * dt)
        t:SetLocalPosition(away)
    end
end
-- ============================================================================


-- ===== Snippet: Line-of-sight check (single ray) ============================
local Math = require("util/Math")
local function CanSee(fromPos, target)
    local goal = target:GetTransform():GetWorldPosition()
    local dir  = Math.Normalize(goal - fromPos)
    local hit  = Raycast(fromPos + dir * 0.1, dir, Math.Length(goal - fromPos))
    return not hit:Hit()   -- nothing blocks -> visible
end
-- ============================================================================


-- ===== Snippet: Vision cone (LOS + angle) ===================================
local Math = require("util/Math")
local function InCone(eyePos, forward, target, halfAngleRad, maxDist)
    local p = target:GetTransform():GetWorldPosition()
    local to = p - eyePos
    local dist = Math.Length(to)
    if dist > maxDist then return false end
    local n  = Math.Normalize(to)
    local dt = Vec3.Dot(n, forward)
    return dt > math.cos(halfAngleRad)
end
-- ============================================================================


-- ===== Snippet: Wander (random walk on terrain) =============================
local Math = require("util/Math")
local goal, t = Vec3.new(0,0,0), nil
function OnInit() t = LuaComponent:GetCurrentEntity():GetTransform() end
function OnUpdate(dt)
    local p = t.LocalPosition
    if Math.Length(p - goal) < 0.5 then
        local r = 6.0
        goal = Vec3.new(p.x + (math.random()*2-1)*r, p.y, p.z + (math.random()*2-1)*r)
        local h = TerrainHeightAt(goal.x, goal.z)
        if h then goal = Vec3.new(goal.x, h, goal.z) end
    end
    t:SetLocalPosition(Vec3.MoveTowards(p, goal, 1.5 * dt))
end
-- ============================================================================
