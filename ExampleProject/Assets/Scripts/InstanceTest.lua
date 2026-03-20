-- InstanceTest.lua
-- Spawns a grid of cubes sharing the same model/material to test instance rendering.
-- Attach to any entity with a LuaScriptComponent.

local GRID_SIZE = 10       -- 10x10x10 = 1000 cubes
local SPACING   = 2.5
local cubes = {}

function OnInit()
    local em = scene:GetEntityManager()

    for x = 0, GRID_SIZE - 1 do
        for y = 0, GRID_SIZE - 1 do
            for z = 0, GRID_SIZE - 1 do
                local name = "Cube_" .. x .. "_" .. y .. "_" .. z
                local e = em:Create(name)

                e:AddTransform()
                local t = e:GetTransform()
                t:SetLocalPosition(Vec3.new(
                    (x - GRID_SIZE * 0.5) * SPACING,
                    y * SPACING + 1.0,
                    (z - GRID_SIZE * 0.5) * SPACING
                ))
                t:SetLocalScale(Vec3.new(1.0, 1.0, 1.0))

                e:AddModelComponent(PrimitiveType.Cube)

                table.insert(cubes, e)
            end
        end
    end

    Log.Info("InstanceTest: spawned " .. #cubes .. " cubes")
end

function OnUpdate(dt)
end
