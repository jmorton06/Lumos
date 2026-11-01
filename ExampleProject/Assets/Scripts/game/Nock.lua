-- First-person nocked arrow: sits low in view while aiming and slides back as
-- the bow is drawn, so the draw has something physical to look at.

local State = require("game/State")
local Bow = require("game/Bow")
local Aim = require("game/Aim")

local M = {}

M._parts = nil
M.HIDE_Y = -10000.0

local SHAFT = Vec4.new(0.55, 0.38, 0.22, 1.0)
local HEAD  = Vec4.new(0.75, 0.78, 0.82, 1.0)
local FLETCH = Vec4.new(0.90, 0.20, 0.18, 1.0)

local function _Build(scene)
    if M._parts or not AddDecorCube then return end
    local hide = Vec3.new(0, M.HIDE_Y, 0)
    -- { entity, offset along the arrow (m), half extents }
    M._parts = {
        { e = AddDecorCube(scene, "NockShaft", hide, Vec3.new(0.014, 0.014, 0.9), SHAFT), along = 0.0 },
        { e = AddDecorCube(scene, "NockHead", hide, Vec3.new(0.035, 0.008, 0.09), HEAD), along = 0.98 },
        { e = AddDecorCube(scene, "NockFletch", hide, Vec3.new(0.003, 0.035, 0.12), FLETCH), along = -0.78 },
    }
end

function M.Cleanup()
    if not M._parts then return end
    for _, p in ipairs(M._parts) do
        if p.e and p.e:Valid() then p.e:Destroy() end
    end
    M._parts = nil
end

local function _Hide()
    for _, p in ipairs(M._parts or {}) do
        if p.e and p.e:Valid() then p.e:GetTransform():SetLocalPosition(Vec3.new(0, M.HIDE_Y, 0)) end
    end
end

function M.Update(dt, ctx, camPos)
    local phase = State.phase
    if phase ~= State.PHASE.Aiming and phase ~= State.PHASE.Charging then
        _Hide()
        return
    end
    _Build(ctx.scene)
    if not M._parts then return end

    local p, y = Aim.PitchRad(), Aim.YawRad()
    local cp, sp, cy, sy = math.cos(p), math.sin(p), math.cos(y), math.sin(y)
    local fwd = { x = cp * sy, y = sp, z = cp * cy }
    -- looking down +Z, screen-right is world -X
    local right = { x = -cy, y = 0.0, z = sy }
    local up = { x = -sp * sy, y = cp, z = -sp * cy }
    local pull = Bow.Power() * 0.7
    -- low and slightly right of centre, tip pointing down the aim line
    local base = 2.2 - pull
    local q = Quat.new(-math.deg(p), math.deg(y), 0.0)
    for _, part in ipairs(M._parts) do
        local d = base + part.along
        local pos = Vec3.new(camPos.x + fwd.x * d + right.x * 0.26 - up.x * 0.46,
                             camPos.y + fwd.y * d + right.y * 0.26 - up.y * 0.46,
                             camPos.z + fwd.z * d + right.z * 0.26 - up.z * 0.46)
        local t = part.e:GetTransform()
        t:SetLocalPosition(pos)
        t:SetLocalOrientation(q)
    end
end

return M
