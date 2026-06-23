-- ============================================================================
-- 07_Spawning — primitives, decor, projectiles, factories
-- ============================================================================

-- ===== Snippet: Quick decor cube / sphere ===================================
local cube = AddDecorCube(scene, "Crate", Vec3.new(0, 0, 0), Vec3.new(1, 1, 1), Vec4.new(0.8, 0.5, 0.2, 1))
local sphere = AddDecorSphere(scene, "Ball", Vec3.new(2, 0.5, 0), 0.5, Vec4.new(0.2, 0.8, 1, 1))
-- ============================================================================


-- ===== Snippet: Physics sphere (mass via inverse mass) ======================
-- inverseMass: 0 = static, 1 = 1kg, 0.5 = 2kg
local ball = AddPhysicsSphere(scene, "Ball", Vec3.new(0, 5, 0), 0.5, 1.0, Vec4.new(1, 1, 1, 1))
-- ============================================================================


-- ===== Snippet: Terrain chunk ===============================================
local t = AddTerrain(scene, Vec3.new(0, 0, 0), 128, 1.0, 5.0)
-- ============================================================================


-- ===== Snippet: Spawn arrow / projectile ====================================
-- AddArrow(scene, pos, velocity, radius?, length?, mass?)
local arrow = AddArrow(scene, Vec3.new(0, 2, 0), Vec3.new(0, 5, 20), 0.05, 1.0, 0.05)
-- ============================================================================


-- ===== Snippet: Target ring entity ==========================================
local target = AddTarget(scene, Vec3.new(0, 1, 30), 1.0, Vec4.new(1, 0.8, 0.2, 1))
-- ============================================================================


-- ===== Snippet: Spawn N enemies in a circle =================================
local Math = require("util/Math")
local function SpawnRing(scene, centre, n, radius)
    for i = 0, n - 1 do
        local ang = i * (math.pi * 2 / n)
        local pos = Vec3.new(centre.x + math.cos(ang) * radius, centre.y, centre.z + math.sin(ang) * radius)
        AddDecorSphere(scene, "Enemy_" .. i, pos, 0.4, Vec4.new(1, 0.2, 0.2, 1))
    end
end
SpawnRing(scene, Vec3.new(0, 1, 0), 8, 6.0)
-- ============================================================================


-- ===== Snippet: Spawn N along a line ========================================
local function SpawnLine(scene, a, b, n)
    for i = 0, n - 1 do
        local t = (n == 1) and 0.0 or (i / (n - 1))
        local p = Vec3.Lerp(a, b, t)
        AddDecorCube(scene, "Post_"..i, p, Vec3.new(0.2, 1, 0.2), Vec4.new(0.6, 0.6, 0.6, 1))
    end
end
SpawnLine(scene, Vec3.new(-5, 0, 0), Vec3.new(5, 0, 0), 11)
-- ============================================================================


-- ===== Snippet: Spawn entity with Lua script attached ======================
local e = scene:GetEntityManager():Create("Smart")
e:AddTransform():SetLocalPosition(Vec3.new(0, 1, 0))
e:AddLuaScriptComponent("//Assets/Scripts/Snippets/00_Skeleton.lua", scene)
-- ============================================================================


-- ===== Snippet: Random scatter inside box ==================================
local function RandomScatter(scene, count, bmin, bmax)
    for i = 1, count do
        local p = Vec3.new(
            bmin.x + math.random() * (bmax.x - bmin.x),
            bmin.y,
            bmin.z + math.random() * (bmax.z - bmin.z))
        AddDecorSphere(scene, "Rock_"..i, p, 0.2 + math.random() * 0.3,
            Vec4.new(0.4 + math.random() * 0.3, 0.3, 0.2, 1))
    end
end
-- ============================================================================


-- ===== Snippet: Highlight an entity (emissive pulse) =======================
SetEntityPulse(targetEntity, Vec4.new(1, 1, 0, 1), 2.0)
-- ============================================================================
