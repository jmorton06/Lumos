-- ============================================================================
-- 06_Camera — orbit, third-person follow, shake, fly
-- ============================================================================
local Math = require("util/Math")

-- ===== Snippet: Find the active camera entity ===============================
local camEntity
function OnInit()
    for e in EachEntity() do
        if e:HasCamera() then camEntity = e; break end
    end
end
-- ============================================================================


-- ===== Snippet: Orbit camera (mouse-drag yaw/pitch + scroll zoom) ===========
local Math = require("util/Math")
local yaw, pitch, dist = 0.0, 0.4, 8.0
local pivot = Vec3.new(0, 1, 0)
local cam

function OnInit()
    for e in EachEntity() do
        if e:HasCamera() then cam = e:GetTransform(); break end
    end
end

function OnUpdate(dt)
    if not cam then return end
    if Input.GetMouseHeld(MouseButton.Right) then
        -- Use Input.GetMousePosition delta if you want; here use scroll only.
    end
    dist = math.max(2.0, dist - Input.GetScrollOffset() * 0.5)
    if Input.GetKeyHeld(Key.Left)  then yaw   = yaw   - dt * 2.0 end
    if Input.GetKeyHeld(Key.Right) then yaw   = yaw   + dt * 2.0 end
    if Input.GetKeyHeld(Key.Up)    then pitch = math.min(1.4, pitch + dt) end
    if Input.GetKeyHeld(Key.Down)  then pitch = math.max(-0.3, pitch - dt) end

    local cp = math.cos(pitch)
    local pos = Vec3.new(
        pivot.x + math.sin(yaw) * cp * dist,
        pivot.y + math.sin(pitch) * dist,
        pivot.z + math.cos(yaw) * cp * dist)
    cam:SetLocalPosition(pos)
    cam:SetLocalOrientation(QuatLookAt(pos, pivot))
end
-- ============================================================================


-- ===== Snippet: Third-person follow with smoothing ==========================
local Math = require("util/Math")
local OFFSET = Vec3.new(0, 4, -7)
local cam, target
function OnInit()
    for e in EachEntity() do if e:HasCamera() then cam = e:GetTransform() end end
    target = GetEntityByName(scene, "Player")
end
function OnUpdate(dt)
    if not (cam and target and target:Valid()) then return end
    local tp = target:GetTransform():GetWorldPosition()
    local goal = tp + OFFSET
    cam:SetLocalPosition(Math.DampVec3(cam.LocalPosition, goal, 5.0, dt))
    cam:SetLocalOrientation(QuatLookAt(cam.LocalPosition, tp + Vec3.new(0, 1, 0)))
end
-- ============================================================================


-- ===== Snippet: Camera shake (additive offset, decays) ======================
local _shake = { amp = 0.0, t = 0.0, total = 0.0 }
local function ShakeTrigger(amp, duration)
    if amp > _shake.amp then _shake.amp = amp end
    _shake.total = duration
    _shake.t     = duration
end

local cam
function OnInit()
    for e in EachEntity() do if e:HasCamera() then cam = e:GetTransform() end end
end

function OnUpdate(dt)
    if not cam or _shake.t <= 0 then return end
    _shake.t = _shake.t - dt
    local k = _shake.t / _shake.total
    local a = _shake.amp * k * k
    local off = Vec3.new(
        (math.random() * 2 - 1) * a,
        (math.random() * 2 - 1) * a,
        (math.random() * 2 - 1) * a)
    local p = cam.LocalPosition
    cam:SetLocalPosition(Vec3.new(p.x + off.x, p.y + off.y, p.z + off.z))
end
-- ============================================================================


-- ===== Snippet: Free-fly camera (WASD + RMB drag) ===========================
local cam, yaw, pitch = nil, 0.0, 0.0
local SPEED = 8.0
local lastMouse

function OnInit()
    for e in EachEntity() do if e:HasCamera() then cam = e:GetTransform() end end
    lastMouse = Input.GetMousePosition()
end

function OnUpdate(dt)
    if not cam then return end
    local mp = Input.GetMousePosition()
    if Input.GetMouseHeld(MouseButton.Right) then
        local dx = (mp.x - lastMouse.x) * 0.005
        local dy = (mp.y - lastMouse.y) * 0.005
        yaw   = yaw + dx
        pitch = math.max(-1.5, math.min(1.5, pitch - dy))
    end
    lastMouse = mp

    local cp = math.cos(pitch)
    local fwd = Vec3.new(math.sin(yaw) * cp, math.sin(pitch), math.cos(yaw) * cp)
    local right = Vec3.Cross(Vec3.new(0,1,0), fwd)

    local move = Vec3.new(0,0,0)
    if Input.GetKeyHeld(Key.W) then move = move + fwd end
    if Input.GetKeyHeld(Key.S) then move = move - fwd end
    if Input.GetKeyHeld(Key.D) then move = move + right end
    if Input.GetKeyHeld(Key.A) then move = move - right end
    if Input.GetKeyHeld(Key.Space)      then move = move + Vec3.new(0, 1, 0) end
    if Input.GetKeyHeld(Key.LeftShift) then move = move - Vec3.new(0, 1, 0) end

    local p = cam.LocalPosition
    cam:SetLocalPosition(p + move * (SPEED * dt))
    cam:SetLocalOrientation(QuatLookAt(p, p + fwd))
end
-- ============================================================================
