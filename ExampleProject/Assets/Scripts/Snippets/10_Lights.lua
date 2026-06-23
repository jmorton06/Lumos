-- ============================================================================
-- 10_Lights — point / spot / directional; flicker; day-night sweep
-- ============================================================================
-- Light.Type: 0=Dir 1=Point 2=Spot  (matches engine enum order; tweak if needed)

-- ===== Snippet: Point light =================================================
local e = scene:GetEntityManager():Create("PointLight")
e:AddTransform():SetLocalPosition(Vec3.new(0, 4, 0))
local l = e:AddLight()
l.Type      = 1
l.Position  = Vec3.new(0, 4, 0)
l.Colour    = Vec4.new(1, 0.9, 0.7, 1)
l.Intensity = 8.0
l.Radius    = 12.0
-- ============================================================================


-- ===== Snippet: Directional sun =============================================
local e = scene:GetEntityManager():Create("Sun")
e:AddTransform()
local l = e:AddLight()
l.Type      = 0
l.Direction = Vec3.new(-0.3, -1.0, -0.4):Normalise()
l.Colour    = Vec4.new(1, 0.95, 0.85, 1)
l.Intensity = 3.0
-- ============================================================================


-- ===== Snippet: Spot light ==================================================
local e = scene:GetEntityManager():Create("Spot")
e:AddTransform()
local l = e:AddLight()
l.Type      = 2
l.Position  = Vec3.new(0, 5, 0)
l.Direction = Vec3.new(0, -1, 0)
l.Colour    = Vec4.new(1, 1, 1, 1)
l.Intensity = 12.0
l.Radius    = 15.0
l.Angle     = 0.5         -- radians, half-angle
-- ============================================================================


-- ===== Snippet: Torch flicker (Perlin-ish) ==================================
local light, _t
function OnInit()
    light = LuaComponent:GetCurrentEntity():GetOrAddLight()
    _t = 0
end
function OnUpdate(dt)
    _t = _t + dt
    local base = 5.0
    local flick = math.sin(_t * 17.0) * 0.6 + math.sin(_t * 31.0) * 0.3
    light.Intensity = base + flick
end
-- ============================================================================


-- ===== Snippet: Day-night sun sweep =========================================
-- Rotates a directional light over a configurable cycle (sec).
local CYCLE_SEC = 60.0
local sunLight, _t
function OnInit()
    sunLight = LuaComponent:GetCurrentEntity():GetOrAddLight()
    sunLight.Type = 0
    _t = 0
end
function OnUpdate(dt)
    _t = (_t + dt) % CYCLE_SEC
    local a = (_t / CYCLE_SEC) * math.pi * 2
    sunLight.Direction = Vec3.new(math.cos(a), -math.sin(a), 0.2):Normalise()
    sunLight.Intensity = math.max(0.0, math.sin(a + math.pi * 0.25)) * 3.0
end
-- ============================================================================


-- ===== Snippet: Sky / horizon colour & fog ==================================
local env  -- Environment* (engine-specific; usually fetched per-scene)
-- (assume env retrieved from scene environment hookup)
env:SetHorizonColour(Vec4.new(0.6, 0.7, 0.9, 1))
env:SetZenithColour (Vec4.new(0.1, 0.2, 0.6, 1))
env:SetSunDirection (Vec3.new(-0.3, -1, -0.4):Normalise())
env:SetFogColour    (Vec4.new(0.7, 0.7, 0.8, 1))
env:SetFogParams    (Vec4.new(0.02, 0.0, 0.0, 0.0))  -- density / etc.
-- ============================================================================


-- ===== Snippet: Pulse glow on hit ===========================================
local l, _hitT
function OnInit()
    l = LuaComponent:GetCurrentEntity():GetOrAddLight()
    _hitT = 0
end
function OnCollision3DBegin(info) _hitT = 0.4 end
function OnUpdate(dt)
    if _hitT > 0 then _hitT = _hitT - dt end
    l.Intensity = 2.0 + math.max(0, _hitT) * 10.0
end
-- ============================================================================
