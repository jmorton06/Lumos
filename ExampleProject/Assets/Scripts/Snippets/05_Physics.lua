-- ============================================================================
-- 05_Physics — rigidbody control, impulses, raycast, triggers
-- ============================================================================

-- ===== Snippet: 3D rigid body construction (cuboid) =========================
local function MakeBox(scene, pos, scale, mass)
    local e = scene:GetEntityManager():Create("Box")
    e:AddTransform():SetLocalPosition(pos)
    e:AddModelComponent():LoadPrimitive(PrimitiveType.Cube)

    local p = RigidBodyParameters.new()
    p.position = pos
    p.scale    = scale
    p.shape    = Shape.Custom
    p.mass     = mass
    p.isStatic = mass <= 0.0
    e:AddRigidBody3DComponent(p)
    return e
end
-- ============================================================================


-- ===== Snippet: Jump (apply impulse on Space) ===============================
local _e
function OnInit() _e = LuaComponent:GetCurrentEntity() end
function OnUpdate(dt)
    if Input.GetKeyPressed(Key.Space) and _e:HasRigidBody3DComponent() then
        local body = _e:GetRigidBody3DComponent():GetRigidBody()
        body:WakeUp()
        body:ApplyImpulse(Vec3.new(0, 6, 0))
    end
end
-- ============================================================================


-- ===== Snippet: Continuous force (rocket thrust) ============================
function OnUpdate(dt)
    local e = LuaComponent:GetCurrentEntity()
    if not e:HasRigidBody3DComponent() then return end
    local body = e:GetRigidBody3DComponent():GetRigidBody()
    body:AddForce(Vec3.new(0, 15, 0) * dt * 60.0)  -- counter gravity-ish
end
-- ============================================================================


-- ===== Snippet: Set / get velocity ==========================================
local function Launch(e, dir, speed)
    if not e:HasRigidBody3DComponent() then return end
    local body = e:GetRigidBody3DComponent():GetRigidBody()
    body:WakeUp()
    body:SetLinearVelocity(dir * speed)
end
-- ============================================================================


-- ===== Snippet: Single raycast (closest hit) ================================
function OnUpdate(dt)
    if Input.GetMouseClicked(MouseButton.Left) then
        local hit = Raycast(Vec3.new(0, 10, 0), Vec3.new(0, -1, 0), 50.0)
        if hit:Hit() then
            Log.Info(string.format("hit at %.2f %.2f %.2f", hit.point.x, hit.point.y, hit.point.z))
            DebugPoint(hit.point, 0.2, Vec4.new(1, 0, 0, 1))
        end
    end
end
-- ============================================================================


-- ===== Snippet: Raycast all (every hit along line) ==========================
local hits = RaycastAll(Vec3.new(0, 5, 0), Vec3.new(1, 0, 0), 30.0)
for i = 1, #hits do
    local h = hits[i]
    DebugPoint(h.point, 0.1, Vec4.new(0, 1, 0, 1))
end
-- ============================================================================


-- ===== Snippet: Terrain height query ========================================
local y = TerrainHeightAt(10.0, 20.0)
if y then
    -- place foliage at (10, y, 20)
end
-- ============================================================================


-- ===== Snippet: Trigger (no collision response) =============================
local p = RigidBodyParameters.new()
p.position = Vec3.new(0, 0, 0)
p.scale    = Vec3.new(2, 2, 2)
p.shape    = Shape.Custom
p.mass     = 0.0
p.isStatic = true
local e = scene:GetEntityManager():Create("Trigger")
e:AddTransform():SetLocalPosition(p.position)
e:AddRigidBody3DComponent(p)
e:GetRigidBody3DComponent():GetRigidBody():SetIsTrigger(true)

function OnCollision3DBegin(info) Log.Info("entered trigger") end
function OnCollision3DEnd(info)   Log.Info("exited trigger")  end
-- ============================================================================


-- ===== Snippet: Physics material presets ====================================
-- bouncy ball
local body = LuaComponent:GetCurrentEntity():GetRigidBody3DComponent():GetRigidBody()
body:SetMaterial(PhysicsMaterial.Bouncy())     -- or .Ice() .Rubber() .Metal() .Wood() .Concrete()
-- ============================================================================


-- ===== Snippet: Lock rotation axes (keep upright) ===========================
-- linear/angular factor: 1 = free, 0 = locked
local body = LuaComponent:GetCurrentEntity():GetRigidBody3DComponent():GetRigidBody()
body:SetAngularFactor(Vec3.new(0, 1, 0))   -- yaw only, prevents falling over
-- ============================================================================


-- ===== Snippet: Collision layer / mask filtering ============================
local body = LuaComponent:GetCurrentEntity():GetRigidBody3DComponent():GetRigidBody()
body:SetCollisionLayer(2)         -- this body is on layer 2
body:SetCollisionMask(1 | 4)      -- only collide with layers 1 and 4
-- ============================================================================


-- ===== Snippet: 2D rigid body (Box2D) =======================================
local p = RigidBodyParameters.new()
p.position = Vec3.new(0, 0, 0)
p.scale    = Vec3.new(0.5, 0.5, 1)
p.shape    = Shape.Square
p.mass     = 1.0
p.isStatic = false
e:AddRigidBody2DComponent(p)
-- ============================================================================


-- ===== Snippet: Custom gravity ==============================================
Physics.SetGravity3D(Vec3.new(0, -3, 0))   -- low-grav
SetB2DGravity(Vec2.new(0, -9.8))            -- 2D
-- ============================================================================
