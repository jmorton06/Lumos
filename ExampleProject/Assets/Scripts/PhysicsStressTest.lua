-- Physics stress test: drops a large grid of dynamic cubes onto a static floor
-- so they pile up and stay in contact. Designed to stress the broadphase
-- (O(n^2) pair finding), narrowphase and the constraint solver simultaneously.
-- Tune COUNT_X/Y/Z to scale the body count.

local COUNT_X = 12   -- bodies per row
local COUNT_Z = 12   -- bodies per column
local COUNT_Y = 5    -- stacked layers   -> total = X*Z*Y dynamic bodies
local SPACING = 1.05 -- < 2*half so AABBs overlap a little (more broadphase pairs)
local HALF    = 0.5  -- cube half-extent

local spawned = 0

local function Spawn()
    local scene = GetCurrentScene()

    -- Static floor.
    AddPhysicsCube(scene, "Floor", Vec3.new(0, -0.5, 0),
        Vec3.new(40, 0.5, 40), 0.0, Vec4.new(0.3, 0.3, 0.35, 1.0))

    local originX = -(COUNT_X - 1) * SPACING * 0.5
    local originZ = -(COUNT_Z - 1) * SPACING * 0.5

    for ly = 0, COUNT_Y - 1 do
        for lx = 0, COUNT_X - 1 do
            for lz = 0, COUNT_Z - 1 do
                local px = originX + lx * SPACING
                local pz = originZ + lz * SPACING
                local py = HALF + 0.05 + ly * (SPACING + 0.1)
                -- colour by layer so the pile reads visually
                local t = ly / COUNT_Y
                AddPhysicsCube(scene, "B" .. spawned,
                    Vec3.new(px, py, pz),
                    Vec3.new(HALF, HALF, HALF),
                    1.0, -- inverseMass (dynamic)
                    Vec4.new(0.2 + t * 0.7, 0.4, 1.0 - t * 0.7, 1.0))
                spawned = spawned + 1
            end
        end
    end

    Log.Info("[StressTest] spawned " .. spawned .. " dynamic bodies (+1 floor)")
end

function OnInit()
    spawned = 0
    Spawn()
end

function OnUpdate(dt)
end

function OnRelease()
end
