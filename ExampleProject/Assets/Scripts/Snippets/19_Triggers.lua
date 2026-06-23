-- ============================================================================
-- 19_Triggers — collision callbacks, AOE damage, zones
-- ============================================================================

-- ===== Snippet: Trigger volume + enter callback =============================
function OnInit()
    local p = RigidBodyParameters.new()
    p.position = Vec3.new(0, 0, 0); p.scale = Vec3.new(3, 3, 3)
    p.shape = Shape.Custom; p.isStatic = true; p.mass = 0.0
    local e = LuaComponent:GetCurrentEntity()
    e:AddRigidBody3DComponent(p)
    e:GetRigidBody3DComponent():GetRigidBody():SetIsTrigger(true)
end
function OnCollision3DBegin(info) Log.Info("enter zone") end
function OnCollision3DEnd(info)   Log.Info("exit zone")  end
-- ============================================================================


-- ===== Snippet: Distance-based AOE (no physics) =============================
local Math = require("util/Math")
local function ExplodeAt(centre, radius, damage)
    for e in EachEntity() do
        if e:HasTransform() and e:HasNameComponent() then
            local p = e:GetTransform():GetWorldPosition()
            if Math.Length(p - centre) <= radius then
                Log.Info("hit: " .. e:GetNameComponent().name)
                -- subtract HP from a stat component, push body, etc.
            end
        end
    end
end
-- ============================================================================


-- ===== Snippet: AOE push (impulse from centre) ==============================
local Math = require("util/Math")
local function AOEPush(centre, radius, force)
    for e in EachEntity() do
        if e:HasRigidBody3DComponent() then
            local body = e:GetRigidBody3DComponent():GetRigidBody()
            local p = body:GetPosition()
            local d = p - centre
            local l = Math.Length(d)
            if l > 0 and l <= radius then
                local strength = (1.0 - l / radius) * force
                body:WakeUp()
                body:ApplyImpulse(Math.Normalize(d) * strength + Vec3.new(0, strength * 0.5, 0))
            end
        end
    end
end
-- ============================================================================


-- ===== Snippet: Damage zone (continuous tick) ===============================
local _inside = false
function OnInit()
    -- assume entity has a trigger body already
end
function OnCollision3DBegin(info) _inside = true  end
function OnCollision3DEnd(info)   _inside = false end

local accum = 0
function OnUpdate(dt)
    if not _inside then return end
    accum = accum + dt
    while accum >= 0.5 do
        accum = accum - 0.5
        Log.Info("tick damage")
    end
end
-- ============================================================================


-- ===== Snippet: Pickup item (destroy on contact) ============================
function OnCollision3DBegin(info)
    local me = LuaComponent:GetCurrentEntity()
    if me:Valid() then me:Destroy() end
    Log.Info("pickup grabbed")
end
-- ============================================================================
