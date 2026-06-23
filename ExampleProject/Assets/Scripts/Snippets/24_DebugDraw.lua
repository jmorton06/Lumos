-- ============================================================================
-- 24_DebugDraw — DebugLine / DebugPoint helpers
-- ============================================================================
-- Globals:
--   DebugLine(a, b, thickness, colourVec4)
--   DebugPoint(pos, radius, colourVec4)

-- ===== Snippet: Draw world axes at origin ===================================
function OnUpdate(dt)
    DebugLine(Vec3.new(0,0,0), Vec3.new(1,0,0), 0.02, Vec4.new(1, 0, 0, 1))
    DebugLine(Vec3.new(0,0,0), Vec3.new(0,1,0), 0.02, Vec4.new(0, 1, 0, 1))
    DebugLine(Vec3.new(0,0,0), Vec3.new(0,0,1), 0.02, Vec4.new(0, 0, 1, 1))
end
-- ============================================================================


-- ===== Snippet: Draw wireframe sphere =======================================
local function DebugSphere(c, r, col, segments)
    segments = segments or 24
    for i = 0, segments - 1 do
        local a = i / segments * math.pi * 2
        local b = (i + 1) / segments * math.pi * 2
        DebugLine(
            Vec3.new(c.x + math.cos(a)*r, c.y, c.z + math.sin(a)*r),
            Vec3.new(c.x + math.cos(b)*r, c.y, c.z + math.sin(b)*r),
            0.02, col)
        DebugLine(
            Vec3.new(c.x + math.cos(a)*r, c.y + math.sin(a)*r, c.z),
            Vec3.new(c.x + math.cos(b)*r, c.y + math.sin(b)*r, c.z),
            0.02, col)
        DebugLine(
            Vec3.new(c.x, c.y + math.cos(a)*r, c.z + math.sin(a)*r),
            Vec3.new(c.x, c.y + math.cos(b)*r, c.z + math.sin(b)*r),
            0.02, col)
    end
end
-- ============================================================================


-- ===== Snippet: Draw wireframe box ==========================================
local function DebugBox(min, max, col)
    local c = col or Vec4.new(1,1,1,1)
    local p = {
        Vec3.new(min.x, min.y, min.z), Vec3.new(max.x, min.y, min.z),
        Vec3.new(max.x, min.y, max.z), Vec3.new(min.x, min.y, max.z),
        Vec3.new(min.x, max.y, min.z), Vec3.new(max.x, max.y, min.z),
        Vec3.new(max.x, max.y, max.z), Vec3.new(min.x, max.y, max.z),
    }
    local edges = {{1,2},{2,3},{3,4},{4,1},{5,6},{6,7},{7,8},{8,5},{1,5},{2,6},{3,7},{4,8}}
    for _, e in ipairs(edges) do DebugLine(p[e[1]], p[e[2]], 0.02, c) end
end
-- ============================================================================


-- ===== Snippet: Draw velocity arrow on rigidbody ============================
function OnUpdate(dt)
    local e = LuaComponent:GetCurrentEntity()
    if not e:HasRigidBody3DComponent() then return end
    local body = e:GetRigidBody3DComponent():GetRigidBody()
    local p = body:GetPosition()
    local v = body:GetLinearVelocity()
    DebugLine(p, p + v * 0.2, 0.03, Vec4.new(1, 1, 0, 1))
end
-- ============================================================================


-- ===== Snippet: Toggle physics debug viz ====================================
function OnUpdate(dt)
    if Input.GetKeyPressed(Key.F2) then
        SetPhysicsDebugFlags(
            PhysicsDebugFlags.COLLISIONVOLUMES |
            PhysicsDebugFlags.AABB |
            PhysicsDebugFlags.LINEARVELOCITY)
    end
    if Input.GetKeyPressed(Key.F3) then
        SetPhysicsDebugFlags(0)
    end
end
-- ============================================================================
