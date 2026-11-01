-- Speed lines: thin glowing streaks scattered around the arrow's path while it
-- flies. They stay put in the world, so with the camera chasing the arrow they
-- rush past - more and longer the faster it goes.

local State = require("game/State")

local M = {}

M.COUNT = 44
M.HIDE_Y = -10000.0
M.COLOUR = Vec4.new(1.0, 0.97, 0.90, 1.0)
M._lines = nil

local function _Build(scene)
    if M._lines or not AddDecorCube then return end
    M._lines = {}
    for i = 1, M.COUNT do
        local e = AddDecorCube(scene, "SpeedLine", Vec3.new(0, M.HIDE_Y, 0), Vec3.new(0.025, 0.025, 1.0), M.COLOUR)
        if e and e:Valid() and SetEntityPulse then SetEntityPulse(e, M.COLOUR, 2.0) end
        M._lines[i] = { e = e, live = false }
    end
end

local function _Hide(l)
    l.live = false
    if l.e and l.e:Valid() then l.e:GetTransform():SetLocalPosition(Vec3.new(0, M.HIDE_Y, 0)) end
end

function M.Cleanup()
    for _, l in ipairs(M._lines or {}) do
        if l.e and l.e:Valid() then l.e:Destroy() end
    end
    M._lines = nil
end

-- pos/vel: arrow centre + velocity this frame (nil when nothing is flying).
function M.Update(dt, ctx, pos, vel)
    local flying = State.phase == State.PHASE.Watching and pos and vel
    if not flying then
        for _, l in ipairs(M._lines or {}) do if l.live then _Hide(l) end end
        return
    end
    _Build(ctx.scene)
    if not M._lines then return end

    local speed = math.sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z)
    if speed < 1.0 then return end
    local d = { x = vel.x / speed, y = vel.y / speed, z = vel.z / speed }
    -- two vectors across the flight direction
    local ux, uy, uz = 0.0, 1.0, 0.0
    if math.abs(d.y) > 0.9 then ux, uy = 1.0, 0.0 end
    local rx, ry, rz = d.y * uz - d.z * uy, d.z * ux - d.x * uz, d.x * uy - d.y * ux
    local rl = math.sqrt(rx * rx + ry * ry + rz * rz)
    rx, ry, rz = rx / rl, ry / rl, rz / rl
    local sx, sy, sz = ry * d.z - rz * d.y, rz * d.x - rx * d.z, rx * d.y - ry * d.x

    local want = math.floor(M.COUNT * math.min(1.0, speed / 110.0))
    local len = math.max(0.6, math.min(7.0, speed * 0.045))
    local q = Quat.new(-math.deg(math.asin(math.max(-1.0, math.min(1.0, d.y)))), math.deg(math.atan(d.x, d.z)), 0.0)

    for i, l in ipairs(M._lines) do
        if l.e and l.e:Valid() then
            if l.live then
                local ox, oy, oz = l.p.x - pos.x, l.p.y - pos.y, l.p.z - pos.z
                -- behind the chase camera (it sits ~11 m back): recycle
                if ox * d.x + oy * d.y + oz * d.z < -16.0 or i > want then _Hide(l) end
            end
            if not l.live and i <= want then
                local ahead = 6.0 + math.random() * 40.0
                local a = math.random() * 6.2832
                local r = 2.5 + math.random() * 7.0
                local cx, cy = math.cos(a) * r, math.sin(a) * r
                l.p = Vec3.new(pos.x + d.x * ahead + rx * cx + sx * cy,
                               pos.y + d.y * ahead + ry * cx + sy * cy,
                               pos.z + d.z * ahead + rz * cx + sz * cy)
                l.live = true
                l.e:GetTransform():SetLocalPosition(l.p)
            end
            if l.live then
                local t = l.e:GetTransform()
                t:SetLocalOrientation(q)
                t:SetLocalScale(Vec3.new(0.035, 0.035, len))
            end
        end
    end
end

return M
